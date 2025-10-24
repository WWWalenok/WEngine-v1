#!/usr/bin/env python3

import re
from cxxheaderparser.types import *
from cxxheaderparser.simple import *
import hashlib

class EnhancedCxxParser:
    def __init__(self):
        self.class_tags = {}  # class_name -> list of tags
        self.method_tags = {}  # (class_name, method_name) -> list of tags
        self.field_tags = {}   # (class_name, field_name) -> list of tags
    
    
    
    def parse_file_with_tags(self, filename):
        with open(filename, 'r', encoding='utf-8') as f:
            source_content = f.read()
        
        source_content_without_tags = self._extract_tags_from_source(source_content)
        
        return parse_string(source_content, cleandoc=True)
    
    def _extract_tags_from_source(self, content):
        """Извлекает теги из макросов в исходном коде"""
        content_without_tags = ""
        lines = content.split('\n')
        t_lines = [line.strip() for line in lines]
        lines = []
        for line in t_lines:
            if len(line):
                lines.append(line)
                
        
        i = 0
        
        while i < len(lines):
            tag_finded = False
            line = lines[i]
            
            # Ищем макросы вида CLASS_TAGS(...)
            class_tags_match = re.match(r'WENGINE_CLASS_TAGS\s*\(\s*(.*?)\s*\)', line)
            if class_tags_match and i + 1 < len(lines):
                tag_finded = True
                tags_str = class_tags_match.group(1)
                tags = [tag.strip() for tag in tags_str.split(',')]
                
                # Следующая строка должна содержать объявление класса
                next_line = lines[i + 1]
                class_match = re.match(r'class\s+(\w+)', next_line)
                if class_match:
                    class_name = class_match.group(1)
                    self.class_tags[class_name] = tags
                    print(f"Найдены теги для класса {class_name}: {tags}")
            
            # Ищем макросы перед методами
            method_tags_match = re.match(r'WENGINE_METHOD_TAGS\s*\(\s*(.*?)\s*\)', line)
            if method_tags_match and i + 1 < len(lines):
                tag_finded = True
                tags_str = method_tags_match.group(1)
                tags = [tag.strip() for tag in tags_str.split(',')]
                
                # Пытаемся найти объявление метода на следующих строках
                for j in range(i + 1, min(i + 5, len(lines))):
                    method_match = re.match(r'static\s+(.+)\s+(\w+)\s*\(', lines[j])
                    if not method_match:
                        method_match = re.match(r'virtual\s+(.+)\s+(\w+)\s*\(', lines[j])
                    if not method_match:
                        method_match = re.match(r'constexpr\s+(.+)\s+(\w+)\s*\(', lines[j])
                    if not method_match:
                        method_match = re.match(r'(.+)\s+(\w+)\s*\(', lines[j])
                    if method_match:
                        return_type = method_match.group(1)
                        method_name = method_match.group(2)
                        
                        # Нужно определить класс, к которому принадлежит метод
                        # Для простоты ищем ближайший класс выше по коду
                        class_name = self._find_nearest_class(lines, j)
                        if class_name:
                            key = (class_name, method_name)
                            self.method_tags[key] = tags
                            print(f"Найдены теги для метода {return_type} {class_name}::{method_name}: {tags}")
                        break
            
            # Ищем макросы перед полями
            field_tags_match = re.match(r'WENGINE_FIELD_TAGS\s*\(\s*(.*?)\s*\)', line)
            if field_tags_match and i + 1 < len(lines):
                tag_finded = True
                tags_str = field_tags_match.group(1)
                tags = [tag.strip() for tag in tags_str.split(',')]
                
                # Пытаемся найти объявление поля на следующих строках
                for j in range(i + 1, min(i + 3, len(lines))):
                    field_match = re.match(r'(\w+(?:\s*::\s*\w+)*(?:\s*<\s*\w+\s*>)?)\s+(\w+)\s*;', lines[j])
                    if field_match:
                        field_type = field_match.group(1)
                        field_name = field_match.group(2)
                        
                        class_name = self._find_nearest_class(lines, j)
                        if class_name:
                            key = (class_name, field_name)
                            self.field_tags[key] = tags
                            print(f"Найдены теги для поля {field_type} {class_name}::{field_name}: {tags}")
                        break
            
            if not tag_finded:
                tag_finded = line.strip()[0] == '#'
            
            if tag_finded == False:
                content_without_tags = content_without_tags + line + "\n"
            i += 1

        return content_without_tags
        
    def _find_nearest_class(self, lines, line_index):
        """Находит ближайший класс выше указанной строки"""
        for i in range(line_index - 1, -1, -1):
            class_match = re.match(r'class\s+(\w+)', lines[i].strip())
            if class_match:
                return class_match.group(1)
        return None
    
    def get_class_tags(self, class_name):
        """Возвращает теги для класса"""
        return self.class_tags.get(class_name, [])
    
    def get_method_tags(self, class_name, method_name):
        """Возвращает теги для метода"""
        return self.method_tags.get((class_name, method_name), [])
    
    def get_field_tags(self, class_name, field_name):
        """Возвращает теги для поля"""
        return self.field_tags.get((class_name, field_name), [])

def generate_guard_name(filename):
    hash_obj = hashlib.md5(filename.encode())
    return f"{filename.replace('.', '_').replace('/', '_').upper()}_{hash_obj.hexdigest()[:16].upper()}_GUARD"

def prepare_header_file(header_path):
    """Подготавливает основной header файл для включения imp-файла"""
    
    with open(header_path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Генерируем guard имя для основного файла
    main_guard = generate_guard_name(header_path)
    
    # Удаляем #pragma once если есть
    content = re.sub(r'^\s*#pragma\s+once\s*$', '', content, flags=re.MULTILINE)
    
    # Проверяем, есть ли уже header guard
    has_guard = re.search(r'#ifndef\s+(\w+)\s*\n#define\s+\1', content)
    
    if has_guard:
        # Если guard есть, включаем imp файл ПЕРЕД ним
        guard_pattern = r'#ifndef\s+(\w+)\s*\n#define\s+\1'
        imp_include = f'#include "{header_path}.imp.hpp"\n\n'
        has_include = re.search(imp_include, content)
        if has_include:
            imp_include = ""
        content = re.sub(guard_pattern, imp_include + r'#ifndef \1\n#define \1', content)
    else:
        # Если guard нет, создаем его с включением imp файла
        imp_include = f'#include "{header_path}.imp.hpp"\n'
        guard_block = f"""#ifndef {main_guard}
#define {main_guard}

"""
        content = imp_include + guard_block + content + f"\n\n#endif // {main_guard}"
    
    # Записываем обратно
    with open(header_path, 'w', encoding='utf-8') as f:
        f.write(content)
    
    return main_guard

def generate_imp_file(header_path, main_guard, meta_part1, meta_part2):
    """Генерирует imp-файл с метаданными"""
    
    imp_guard = generate_guard_name(header_path + ".imp.hpp")
    
    imp_content = f"""#ifndef {imp_guard}
#define {imp_guard}

// Meta part 1 - до включения основного файла
{meta_part1}

#include "{header_path}"

// Meta part 2 - после включения основного файла  
{meta_part2}

#endif // {imp_guard}
"""
    
    imp_path = header_path + ".imp.hpp"
    with open(imp_path, 'w', encoding='utf-8') as f:
        f.write(imp_content)
    
    return imp_path

def process_header_with_metadata(header_path, meta_generator_func):
    """Основная функция обработки header файла с метаданными"""
    
    # 1. Подготавливаем основной файл
    main_guard = prepare_header_file(header_path)
    
    # 2. Генерируем метаданные (это должна быть ваша функция)
    meta_part1, meta_part2 = meta_generator_func(header_path)
    
    # 3. Создаем imp-файл
    imp_path = generate_imp_file(header_path, main_guard, meta_part1, meta_part2)

def get_name(val):
    if hasattr(val, 'format'):
        return val.format()
    if hasattr(val, 'ptr_to') and hasattr(val.ptr_to, 'format'):
        return val.ptr_to.format() + "*"
    if hasattr(val, 'typename') and hasattr(val.typename, 'format'):
        return val.typename.format()
    if hasattr(val, 'name') and hasattr(val.name, 'format'):
        return val.name.format()
    if hasattr(val, 'name'):
        return str(val.name)
    elif hasattr(val, 'show'):
        return val.show()
    else:
        return str(val)

def example_meta_generator(header_path):
    """Пример генератора метаданных на основе вашего парсера"""
    
    # Используем ваш EnhancedCxxParser здесь
    parser = EnhancedCxxParser()
    result = parser.parse_file_with_tags(header_path)
    
    meta_part1 = """
#include \"core.h\"

#ifndef WENGINE_CLASS_TAGS
#define WENGINE_CLASS_TAGS(...)
#endif WENGINE_CLASS_TAGS

#ifndef WENGINE_FIELD_TAGS
#define WENGINE_FIELD_TAGS(...)
#endif WENGINE_FIELD_TAGS

#ifndef WENGINE_METHOD_TAGS
#define WENGINE_METHOD_TAGS(...)
#endif WENGINE_METHOD_TAGS
"""
    meta_part2 = "// Post-class metadata\n// Runtime info, serialization functions etc."
    
    # Генерируем реальные метаданные на основе распарсенных классов
    for class_def in result.namespace.classes:
        if isinstance(class_def, ClassScope) and isinstance(class_def.class_decl, ClassDecl):
            class_name = get_name(class_def.class_decl).replace("class", "").replace("struct", "").strip()
            class_tags = parser.get_class_tags(class_name)
            # Meta part 2: функции, требующие полного объявления класса
            if "nonserializable" not in class_tags:
                serialize_metod = None
                deserialize_metod = None
                for method in class_def.methods:
                    metod_name = get_name(method)
                    method_tags = parser.get_method_tags(class_name, metod_name)
                    if not serialize_metod and "binserializer" in method_tags:
                        serialize_metod = metod_name
                    
                    if not deserialize_metod and "bindeserializer" in method_tags:
                        deserialize_metod = metod_name
                    
                    if serialize_metod and deserialize_metod:
                        break
                
                if serialize_metod and deserialize_metod:
                    meta_part2 += f"""
DECLARE_DATA_TYPE({class_name});
DECLARE_DATA_SERIALIZER_FUNC({class_name}) {{
    DECLARE_DATA_SERIALIZER_DEFAULT_HEADER({class_name});
    return {class_name}::{serialize_metod}(serealized, deserealized);
}}
DECLARE_DATA_DESERIALIZER_FUNC({class_name}) {{
    DECLARE_DATA_DESERIALIZER_DEFAULT_HEADER({class_name});
    return {class_name}::{deserialize_metod}(serealized, deserealized);
}}
DECLARE_DATA_SERIALIZABLE({class_name});
"""
                else:
                    public_class_fields = ""
                    for field in class_def.fields:
                        if field.access == "public":
                            public_class_fields = public_class_fields + ", " + get_name(field)
                    if len(public_class_fields) > 0:
                        meta_part2 += f"""\nDECLARE_DATA_STRUCT_AND_SERIALIZE({class_name}{public_class_fields})\n"""
                
    
    return meta_part1, meta_part2

if __name__ == "__main__":
    header_file = "WObject.h"
    process_header_with_metadata(header_file, example_meta_generator)
        