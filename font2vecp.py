#!/usr/bin/env python3
"""
Конвертер TTF/OTF -> VecP-шрифт (.wfn0).

Использование:
    python font2vecp.py input.ttf output.wfn0
"""
import struct
import sys
from fontTools.ttLib import TTFont
from fontTools.pens.recordingPen import DecomposingRecordingPen

# --- e_elememt_type ---
E_MOVE_A   = 0
E_CLOSE    = 2
E_LINE_A   = 5
E_BEZIE2_A = 11
E_BEZIE3_A = 15

# --- Магия ---
FONT_F_TYPE   = 0x57464E30   # 'WFN0'
VECTOR_F_TYPE = 0x564C5045   # 'VLPE'

# --- Размеры заголовков ---
VECTOR_HDR_SIZE  = 88
COUNTER_HDR_SIZE = 112
GLYF_HDR_SIZE    = 20
FONT_HDR_SIZE    = 60




def put_u32(buf, v): buf += struct.pack('<I', v & 0xFFFFFFFF)
def put_i32(buf, v): buf += struct.pack('<i', v)
def put_f64(buf, v): buf += struct.pack('<d', v)


def pack_xy(data_buf, x, y):
    data_buf += struct.pack('<dd', x, y)
    return data_buf


def subpaths(ops):
    """Разбивает записанные операции пера на подконтуры (по moveTo)."""
    out, cur = [], []
    for op, args in ops:
        if op == 'moveTo':
            if cur:
                out.append(cur)
            cur = [(op, args)]
        else:
            cur.append((op, args))
    if cur:
        out.append(cur)
    return out


def pack_subpath(ops):
    """Возвращает (elements_bytes, data_bytes) для одного контура."""
    elements = bytearray()
    data     = bytearray()
    cur      = (0.0, 0.0)
    start    = (0.0, 0.0)

    for op, args in ops:
        if op == 'moveTo':
            x, y = args[0]
            elements.append(E_MOVE_A)
            data = pack_xy(data, x, y)
            cur = start = (x, y)

        elif op == 'lineTo':
            x, y = args[0]
            elements.append(E_LINE_A)
            data = pack_xy(data, x, y)
            cur = (x, y)

        elif op == 'qCurveTo':
            pts = list(args)
            if pts[-1] is None:
                # TrueType: контур заканчивается off-curve точкой
                pts = pts[:-1]
                end = ((pts[-1][0] + start[0]) * 0.5,
                       (pts[-1][1] + start[1]) * 0.5)
                ctrls = pts
            else:
                end   = pts[-1]
                ctrls = pts[:-1]

            if len(ctrls) == 1:
                c = ctrls[0]
                elements.append(E_BEZIE2_A)
                data = pack_xy(data, c[0], c[1])
                data = pack_xy(data, end[0], end[1])
            else:
                for i, c in enumerate(ctrls):
                    if i < len(ctrls) - 1:
                        nx, ny = ctrls[i + 1]
                        mx, my = (c[0] + nx) * 0.5, (c[1] + ny) * 0.5
                        elements.append(E_BEZIE2_A)
                        data = pack_xy(data, c[0], c[1])
                        data = pack_xy(data, mx, my)
                    else:
                        elements.append(E_BEZIE2_A)
                        data = pack_xy(data, c[0], c[1])
                        data = pack_xy(data, end[0], end[1])
            cur = end

        elif op == 'curveTo':
            c1, c2, end = args[0], args[1], args[2]
            elements.append(E_BEZIE3_A)
            data = pack_xy(data, c1[0], c1[1])
            data = pack_xy(data, c2[0], c2[1])
            data = pack_xy(data, end[0], end[1])
            cur = end

        elif op == 'closePath':
            elements.append(E_CLOSE)
            cur = start

        elif op == 'endPath':
            pass

    return bytes(elements), bytes(data)


def pack_counter(elements, data):
    total = COUNTER_HDR_SIZE + len(elements) + len(data)

    hdr = bytearray()
    put_u32(hdr, total)          # size
    put_u32(hdr, 0)              # corner_type
    put_f64(hdr, 0.0)            # border_inner
    put_f64(hdr, 0.0)            # border_outer
    put_f64(hdr, 1.0)            # transparent
    put_u32(hdr, 0x000000)       # background_color
    put_f64(hdr, 1.0)            # border_transparent  <-- новое поле
    put_u32(hdr, 0x000000)       # border_color
    for v in (1.0, 0.0, 0.0, 0.0, 1.0, 0.0):
        put_f64(hdr, v)          # трансформ (identity)
    put_u32(hdr, 0)              # metadata_size
    put_u32(hdr, 0)              # properties_size
    put_u32(hdr, len(elements))  # elements_size
    put_u32(hdr, len(data) // 8) # data_size

    assert len(hdr) == COUNTER_HDR_SIZE, len(hdr)
    return bytes(hdr) + elements + data


def pack_vector(counters_bytes, width=0, height=0, vtype=0):
    """Собирает vector_t (заголовок + все счётчики)."""
    total = VECTOR_HDR_SIZE + len(counters_bytes)

    hdr = bytearray()
    put_u32(hdr, VECTOR_F_TYPE)   # f_type
    put_u32(hdr, 0)               # format
    put_u32(hdr, 0)               # ts
    put_u32(hdr, total)           # size
    put_u32(hdr, vtype)           # type
    put_u32(hdr, width)           # width
    put_u32(hdr, height)          # heigt (sic!)
    for v in (1.0, 0.0, 0.0, 0.0, 1.0, 0.0):
        put_f64(hdr, v)           # transform
    put_u32(hdr, 0)               # metadata_size
    put_u32(hdr, 0)               # properties_size
    put_u32(hdr, len(counters_bytes) and counters_bytes and 0)  # placeholder
    # заменим счётчик ниже
    hdr[-4:] = struct.pack('<I', _count_counters(counters_bytes))
    assert len(hdr) == VECTOR_HDR_SIZE, len(hdr)
    return bytes(hdr) + counters_bytes


def _count_counters(counters_bytes):
    """Считает число counter_t в уже упакованном блоке."""
    n = 0
    off = 0
    while off < len(counters_bytes):
        (size,) = struct.unpack_from('<I', counters_bytes, off)
        off += size
        n += 1
    return n


def pack_glyf(cp, adv, lsb, flags, counter_blob):
    hdr = struct.pack('<IiiII', cp, int(adv), int(lsb), int(flags), len(counter_blob))
    assert len(hdr) == GLYF_HDR_SIZE, len(hdr)
    return hdr + counter_blob


def convert_font(src_path, dst_path, *, flip_y=True):
    font = TTFont(src_path, fontNumber=0)
    glyphset = font.getGlyphSet()

    upm       = font['head'].unitsPerEm
    ascender  = font['hhea'].ascent
    descender = font['hhea'].descent
    line_gap  = font['hhea'].lineGap
    head      = font['head']
    x_min, y_min = int(head.xMin), int(head.yMin)
    x_max, y_max = int(head.xMax), int(head.yMax)

    cmap = font.getBestCmap()
    if 0 not in cmap and '.notdef' in glyphset:
        cmap = dict(cmap)
        cmap[0] = '.notdef'

    metrics = font['hmtx'].metrics

    glyf_blobs = []
    for cp, name in sorted(cmap.items()):
        if name not in glyphset:
            continue

        pen = DecomposingRecordingPen(glyphset)
        glyphset[name].draw(pen)
        raw_ops = _flip_y(pen.value) if flip_y else pen.value

        # === ОДИН контур: склеиваем все подконтуры в один counter ===
        elements = bytearray()
        data     = bytearray()
        for sp in subpaths(raw_ops):
            e, d = pack_subpath(sp)
            elements += e
            data     += d

        if not elements:
            continue

        counter_blob = pack_counter(bytes(elements), bytes(data))
        adv, lsb     = metrics.get(name, (upm, 0))
        glyf_blobs.append(pack_glyf(cp, adv, lsb, 0, counter_blob))

    body = b''.join(glyf_blobs)

    header = bytearray()
    put_u32(header, FONT_F_TYPE)             # 'WFN0'
    put_u32(header, 1)                       # format
    put_u32(header, 0)                       # ts
    put_u32(header, FONT_HDR_SIZE + len(body))  # size
    put_i32(header, upm)
    put_i32(header, ascender)
    put_i32(header, descender)
    put_i32(header, line_gap)
    put_i32(header, x_min); put_i32(header, y_min)
    put_i32(header, x_max); put_i32(header, y_max)
    put_u32(header, 0)                       # metadata_size
    put_u32(header, 0)                       # properties_size
    put_u32(header, len(glyf_blobs))         # glyfs_size
    assert len(header) == FONT_HDR_SIZE, len(header)

    with open(dst_path, 'wb') as f:
        f.write(header)
        f.write(body)

    print(f"[OK] {src_path} -> {dst_path}  ({len(glyf_blobs)} глифов, UPM={upm})")


def _flip_y(ops):
    """Инвертирует Y (TrueType Y-вверх -> SVG Y-вниз)."""
    out = []
    for op, args in ops:
        new_args = []
        for a in args:
            if isinstance(a, tuple):
                new_args.append((a[0], -a[1]))
            elif a is None:
                new_args.append(None)
            else:
                new_args.append(a)
        out.append((op, tuple(new_args)))
    return out


if __name__ == '__main__':
    if len(sys.argv) != 3:
        print("usage: font2vecp.py input.ttf output.wfn0")
        sys.exit(2)
    convert_font(sys.argv[1], sys.argv[2])