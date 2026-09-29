#pragma once


#include <math.h>
#include <vector>
#include <string>

/*!
 * @brief Fixed-size mathematical vector template for 2D, 3D, and 4D operations
 * @tparam T The element type (typically float, double, or int)
 * @tparam S The vector dimension size
 *
 * @note Provides common vector operations including arithmetic, dot product,
 *       magnitude calculation, and string conversion. Optimized for graphics
 *       and mathematical computations with fixed-size vectors.
 */
template<typename T, uint16_t S>
struct MVector
{
    T _data[S]; ///< Internal array storing vector components

    /*! @brief Default constructor initializes all elements to zero */
    MVector<T, S>()
    {
        for (int i = 0; i < S; i++)
            _data[i] = 0;
    }

    /*! @brief Default constructor */
    MVector<T, S>(const MVector<T, S> &other)
    {
        for (int i = 0; i < S; i++)
            _data[i] = other._data[i];
    }

    /*!
     * @brief Constructor from raw array
     * @param data Pointer to array of exactly S elements
     */
    MVector<T, S>(const T *data)
    {
        for (int i = 0; i < S; i++)
            _data[i] = data[i];
    }

    /*!
     * @brief Variadic constructor for explicit element initialization
     * @tparam ARGS Parameter pack must contain exactly S elements of type T
     * @param args Exactly S values to initialize vector elements
     */
    template<typename... ARGS,
      typename Check = std::enable_if_t<sizeof...(ARGS) == S && std::conjunction_v<std::is_same<T, ARGS>...>>>
    MVector<T, S>(ARGS... args)
    {
        T data[S]{args...};
        for (int i = 0; i < S; i++)
            _data[i] = data[i];
    }

    /*!
     * @brief Constructor from initializer list
     * @param data Initializer list with up to S elements
     * @note If list contains fewer than S elements, remaining elements are zero-initialized
     */
    MVector<T, S>(std::initializer_list<T> data)
    {
        int i = 0;
        for (const auto &e : data)
        {
            if (i >= S)
                break;
            _data[i] = e;
            ++i;
        }
        for (; i < S; i++)
            _data[i] = 0;
    }

    /*! @brief Unary negation operator */
    MVector<T, S> operator-() const
    {
        T data[S];
        for (int i = 0; i < S; i++)
            data[i] = _data[i] * -1;
        return MVector<T, S>(data);
    }

    /*! @brief Unary plus operator (returns copy) */
    MVector<T, S> operator+() const
    {
        T data[S];
        for (int i = 0; i < S; i++)
            data[i] = _data[i];
        return MVector<T, S>(data);
    }

    /*! @brief Vector addition */
    MVector<T, S> operator+(const MVector<T, S> &other) const
    {
        T data[S];
        for (int i = 0; i < S; i++)
            data[i] = _data[i] + other._data[i];
        return MVector<T, S>(data);
    }

    /*! @brief Vector subtraction */
    MVector<T, S> operator-(const MVector<T, S> &other) const
    {
        T data[S];
        for (int i = 0; i < S; i++)
            data[i] = _data[i] - other._data[i];
        return MVector<T, S>(data);
    }

    /*!
     * @brief Scalar multiplication
     * @param other Scalar value to multiply each component by
     */
    MVector<T, S> operator*(const T &other) const
    {
        T data[S];
        for (int i = 0; i < S; i++)
            data[i] = _data[i] * other;
        return MVector<T, S>(data);
    }

    /*!
     * @brief Dot product operation
     * @param other Vector to compute dot product with
     * @return Scalar result of dot product
     * @note Implements component-wise multiplication and summation
     */
    T operator*(const MVector<T, S> &other) const
    {
        T data = _data[0] * other._data[0];
        for (int i = 1; i < S; i++)
            data = data + _data[i] * other._data[i];
        return data;
    }

    /*!
     * @brief Scalar division
     * @param other Scalar value to divide each component by
     */
    MVector<T, S> operator/(const T &other) const
    {
        T data[S];
        for (int i = 0; i < S; i++)
            data[i] = _data[i] / other;
        return MVector<T, S>(data);
    }

    /*!
     * @brief Magnitude (length) calculation
     * @return Euclidean norm of the vector
     * @note Uses appropriate sqrt function for float/double, falls back to standard sqrt for other types
     */
    T operator!() const
    {
        T ret = _data[0] * _data[0];
        for (int i = 1; i < S; i++)
            ret += _data[i] * _data[i];
        if constexpr (std::is_same_v<T, float>)
            return sqrtf(ret);
        else if constexpr (std::is_same_v<T, double>)
            return sqrt(ret);
        else
            return (T)(sqrt(ret));
    }

    bool operator==(const MVector<T, S> &other) const
    {
        for (int i = 0; i < S; i++)
            if (other._data[i] != _data[i])
                return false;
        return true;
    }

    bool operator!=(const MVector<T, S> &other) const
    {
        for (int i = 0; i < S; i++)
            if (other._data[i] != _data[i])
                return true;
        return false;
    }

    /*!
     * @brief Element access with bounds checking
     * @param i Index of element to access (0-based)
     * @return Reference to element at index i % S
     */
    T &operator[](int i)
    {
        return _data[i % S];
    }

    /*!
     * @brief Element access with bounds checking
     * @param i Index of element to access (0-based)
     * @return Reference to element at index i % S
     */
    const T &operator[](int i) const
    {
        return _data[i % S];
    }

    /*!
     * @brief String representation of vector
     * @return String in format "(x, y, z, ...)" with comma-separated values
     */
    operator std::string() const
    {
        std::string ret = "(";
        for (int i = 0; i < S; i++)
        {
            ret += std::to_string(_data[i]);
            if (i != S - 1)
                ret += ", ";
        }
        ret += ")";
        return ret;
    }

    /*!
     * @brief Normalize yhis vector
     * @return This vector
     */
    MVector &Normalize()
    {
        T ret = _data[0] * _data[0];
        for (int i = 1; i < S; i++)
            ret += _data[i] * _data[i];
        if(ret == 0)
            return *this;

        if constexpr (std::is_same_v<T, float>)
        {
            ret = sqrtf(ret);
            for (int i = 0; i < S; i++)
                _data[i] = _data[i] / ret;
        }
        else if constexpr (std::is_same_v<T, double>)
        {
            ret = sqrt(ret);
            for (int i = 0; i < S; i++)
                _data[i] = _data[i] / ret;
        }
        else
        {
            ret = (T)(sqrt(ret));
            for (int i = 0; i < S; i++)
                _data[i] = _data[i] / ret;
        }
        return *this;
    }


    /*!
     * @brief Get new normalized vector
     * @return New normalized vector
     */
    MVector GetNormalized() const
    {
        return MVector(*this).Normalize();
    }
};

template<typename T, uint16_t S>
struct std::hash<MVector<T, S>>
{

    constexpr size_t operator()(const MVector<T, S> &val) const
    {
        size_t hash = 0x811c9dc5;
        size_t h1 = 0x959954B9;
        unsigned char *name = (unsigned char *)val._data;
        while (*name)
        {
            h1 = *name + (h1 << 5) + (h1 << 7) + (h1 << 17) - h1;
            hash ^= (unsigned char)*name++;
            hash *= 0x01000193;
        }

        return (hash | (h1 << 32)) | 0x1ULL;
    }
};

typedef MVector<float, 2> MVector2f;
typedef MVector<float, 3> MVector3f;
typedef MVector<float, 4> MVector4f;
typedef MVector<int, 4> MVector4i;

/*!
 * @brief Cross product operation for 3D vectors
 * @param a First vector
 * @param b Second vector
 * @return Cross product result (a × b)
 * @note Specifically defined for MVector3f, implements standard cross product formula
 */
static MVector3f operator/(const MVector3f &a, const MVector3f &b)
{
    return MVector3f({a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]});
}


template<typename T, uint16_t S>
struct MMatrix
{
    T m[S][S] = {0};

    MMatrix<T, S>() = default;

    // Создание единичной матрицы
    static constexpr MMatrix<T, S> identity()
    {
        MMatrix<T, S> mat;
        for(int i = 0; i < S; ++i)
            mat.m[i][i] = 1;
        return mat;
    }

    MMatrix<T, S> invert() const
    {
        MMatrix<T, S> result;

        if constexpr (S == 1) {
            if (std::abs(m[0][0]) > T(1e-6)) {
                result.m[0][0] = T(1) / m[0][0];
            }
        } 
        else if constexpr (S == 2) {
            T det = m[0][0] * m[1][1] - m[0][1] * m[1][0];
            if (std::abs(det) > T(1e-6)) {
                T invDet = T(1) / det;
                result.m[0][0] =  m[1][1] * invDet;
                result.m[0][1] = -m[0][1] * invDet;
                result.m[1][0] = -m[1][0] * invDet;
                result.m[1][1] =  m[0][0] * invDet;
            }
        } 
        else if constexpr (S == 3) {
            T a = m[0][0], b = m[0][1], c = m[0][2];
            T d = m[1][0], e = m[1][1], f = m[1][2];
            T g = m[2][0], h = m[2][1], i = m[2][2];

            T A =  (e * i - f * h);
            T B = -(d * i - f * g);
            T C =  (d * h - e * g);
            
            T det = a * A + b * B + c * C;
            
            if (std::abs(det) > T(1e-6)) {
                T invDet = T(1) / det;
                
                T D = -(b * i - c * h);
                T E =  (a * i - c * g);
                T F = -(a * h - b * g);
                
                T G =  (b * f - c * e);
                T H = -(a * f - c * d);
                T I =  (a * e - b * d);
                
                result.m[0][0] = A * invDet;
                result.m[0][1] = D * invDet;
                result.m[0][2] = G * invDet;
                
                result.m[1][0] = B * invDet;
                result.m[1][1] = E * invDet;
                result.m[1][2] = H * invDet;
                
                result.m[2][0] = C * invDet;
                result.m[2][1] = F * invDet;
                result.m[2][2] = I * invDet;
            }
        } 
        else if constexpr (S == 4) {
            // Оптимизированный Гаусс-Жордан для 4x4 (разворачивается компилятором)
            T a[4][8];
            for (int r = 0; r < 4; ++r) {
                for (int c = 0; c < 4; ++c) a[r][c] = m[r][c];
                for (int c = 4; c < 8; ++c) a[r][c] = (r == c - 4) ? T(1) : T(0);
            }
            
            for (int i = 0; i < 4; ++i) {
                // Частичный пиотинг
                int pivot = i;
                T maxval = std::abs(a[i][i]);
                for (int r = i + 1; r < 4; ++r) {
                    if (std::abs(a[r][i]) > maxval) {
                        maxval = std::abs(a[r][i]);
                        pivot = r;
                    }
                }
                if (pivot != i) {
                    for (int c = 0; c < 8; ++c) std::swap(a[i][c], a[pivot][c]);
                }
                
                if (std::abs(a[i][i]) < T(1e-6)) return identity();
                
                T invPivot = T(1) / a[i][i];
                for (int c = 0; c < 8; ++c) a[i][c] *= invPivot;
                
                for (int r = 0; r < 4; ++r) {
                    if (r != i) {
                        T factor = a[r][i];
                        for (int c = 0; c < 8; ++c) {
                            a[r][c] -= factor * a[i][c];
                        }
                    }
                }
            }
            
            for (int r = 0; r < 4; ++r) {
                for (int c = 0; c < 4; ++c) {
                    result.m[r][c] = a[r][c + 4];
                }
            }
        } 
        else {
            // Обобщенный метод Гаусса-Жордана для любых других размерностей
            T a[S][2 * S];
            for (int i = 0; i < S; ++i) {
                for (int j = 0; j < S; ++j) a[i][j] = m[i][j];
                for (int j = 0; j < S; ++j) a[i][j + S] = (i == j) ? T(1) : T(0);
            }
            
            for (int i = 0; i < S; ++i) {
                int pivot = i;
                T maxval = std::abs(a[i][i]);
                for (int k = i + 1; k < S; ++k) {
                    if (std::abs(a[k][i]) > maxval) {
                        maxval = std::abs(a[k][i]);
                        pivot = k;
                    }
                }
                
                if (pivot != i) {
                    for (int k = 0; k < 2 * S; ++k) std::swap(a[i][k], a[pivot][k]);
                }
                
                if (std::abs(a[i][i]) < T(1e-6)) {
                    return identity();
                }
                
                T invPivot = T(1) / a[i][i];
                for (int k = 0; k < 2 * S; ++k) a[i][k] *= invPivot;
                
                for (int j = 0; j < S; ++j) {
                    if (j != i) {
                        T factor = a[j][i];
                        for (int k = 0; k < 2 * S; ++k) {
                            a[j][k] -= factor * a[i][k];
                        }
                    }
                }
            }
            
            for (int i = 0; i < S; ++i) {
                for (int j = 0; j < S; ++j) {
                    result.m[i][j] = a[i][j + S];
                }
            }
        }

        return result;
    }

    // Умножение матриц
    MMatrix<T, S> operator*(const MMatrix<T, S> &other) const
    {
        MMatrix<T, S> result;
        for (int i = 0; i < S; ++i)
        {
            for (int j = 0; j < S; ++j)
            {
                result.m[i][j] = 0;
                for (int k = 0; k < S; ++k)
                {
                    result.m[i][j] += m[i][k] * other.m[k][j];
                }
            }
        }
        return result;
    }

    // Умножение матрицы на вектор
    MVector<T, S> operator*(const MVector<T, S> &vec) const
    {
        float result[S] = {0};
        for (int i = 0; i < S; ++i)
        {
            for (int j = 0; j < S; ++j)
            {
                result[i] += m[i][j] * vec[j];
            }
        }
        return MVector<T, S>(result);
    }

    // Преобразование точки (автоматическое добавление w=1)
    MVector<T, S> transformPoint(const MVector<T, S> &point) const
    {
        MVector<T, S> temp = point;
        temp = *this * temp;
        if (temp[S - 1] != 0)
            return temp / temp[S - 1];
        return temp;
    }

    MVector<T, S> transformVector(const MVector<T, S> &vec) const
    {
        return *this * vec;
    }

    // Создание матрицы переноса
    template<uint16_t V>
    static MMatrix<T, S> translate(const MVector<T, V> &translation)
    {
        static_assert(V <= S, "");
        MMatrix<T, S> mat;
        for (int i = 0; i < S; i++)
            mat.m[i][i] = 1;

        for (int i = 0; i < S - 1; i++)
            if (i < V)
                mat.m[i][S - 1] = translation[i];
            else if (i == S - 1)
                mat.m[i][S - 1] = 1;
            else
                mat.m[i][S - 1] = 0;
        return mat;
    }

    // Создание матрицы масштабирования
    static MMatrix<T, S> scale(const MVector<T, S - 1> &scaling)
    {
        MMatrix<T, S> mat;
        for (int i = 0; i < S - 1; i++)
            if (i < V)
                mat.m[i][i] = translation[i];
            else if (i == S - 1)
                mat.m[i][i] = 1;
            else
                mat.m[i][i] = 1;
        return mat;
    }
};


typedef MMatrix<float, 4> MMatrix4f;
typedef MMatrix<float, 3> MMatrix3f;
typedef MMatrix<float, 2> MMatrix2f;
typedef MMatrix<int, 4> MMatrix4i;

namespace wm
{
constexpr double PI = 3.1415926535897932384626433832795;

static MMatrix4f RotateX(float angle)
{
    float c = cos(angle);
    float s = sin(angle);
    MMatrix4f mat = MMatrix4f::identity();
    // столбцы осей
    mat.m[1][1] = c;
    mat.m[2][1] = -s;
    mat.m[1][2] = s;
    mat.m[2][2] = c;
    return mat;
}

static MMatrix4f RotateY(float angle)
{
    float c = cos(angle);
    float s = sin(angle);
    MMatrix4f mat = MMatrix4f::identity();
    mat.m[0][0] = c;
    mat.m[2][0] = s;
    mat.m[0][2] = -s;
    mat.m[2][2] = c;
    return mat;
}

static MMatrix4f RotateZ(float angle)
{
    float c = cos(angle);
    float s = sin(angle);
    MMatrix4f mat = MMatrix4f::identity();
    mat.m[0][0] = c;
    mat.m[1][0] = -s;
    mat.m[0][1] = s;
    mat.m[1][1] = c;
    return mat;
}

static MMatrix4f Perspective(float fov, float aspect, float _near, float _far)
{
    float f = 1.0f / tan(fov / 2.0f);
    float range = _near - _far;

    MMatrix4f mat;
    mat.m[0][0] = f / aspect;
    mat.m[1][1] = f;
    mat.m[2][2] = (_far + _near) / range;
    mat.m[2][3] = (2.0f * _far * _near) / range; // столбец 3
    mat.m[3][2] = -1.0f;
    mat.m[3][3] = 0.0f;
    return mat;
}

static MMatrix4f LookAt(const MVector3f &eye, const MVector3f &center, const MVector3f &up)
{
    MVector3f f = (center - eye);
    f = f / !f; // forward

    MVector3f s = f / up; // right
    s = s / !s;

    MVector3f u = s / f;  // up

    MMatrix4f mat = MMatrix4f::identity();

    // row 0 = right
    mat.m[0][0] =  s[0];
    mat.m[0][1] =  s[1];
    mat.m[0][2] =  s[2];

    // row 1 = up
    mat.m[1][0] =  u[0];
    mat.m[1][1] =  u[1];
    mat.m[1][2] =  u[2];

    // row 2 = -forward
    mat.m[2][0] = -f[0];
    mat.m[2][1] = -f[1];
    mat.m[2][2] = -f[2];

    // col 3 = translation
    mat.m[0][3] = -s[0] * eye[0] - s[1] * eye[1] - s[2] * eye[2];
    mat.m[1][3] = -u[0] * eye[0] - u[1] * eye[1] - u[2] * eye[2];
    mat.m[2][3] =  f[0] * eye[0] + f[1] * eye[1] + f[2] * eye[2];

    return mat;
}

static MVector3f GetTranslation(const MMatrix4f &mat)
{
    return MVector3f(mat.m[0][3], mat.m[1][3], mat.m[2][3]);
}

static MVector3f GetScale(const MMatrix4f &mat)
{
    // длины столбцов 0, 1, 2
    return MVector3f(!MVector3f(mat.m[0][0], mat.m[1][0], mat.m[2][0]),
      !MVector3f(mat.m[0][1], mat.m[1][1], mat.m[2][1]),
      !MVector3f(mat.m[0][2], mat.m[1][2], mat.m[2][2]));
}

static MVector4f GetRotation(const MMatrix4f &mat)
{
    // Извлекаем столбцы и удаляем масштаб
    MVector3f col0(mat.m[0][0], mat.m[1][0], mat.m[2][0]);
    MVector3f col1(mat.m[0][1], mat.m[1][1], mat.m[2][1]);
    MVector3f col2(mat.m[0][2], mat.m[1][2], mat.m[2][2]);

    float len0 = !col0;
    float len1 = !col1;
    float len2 = !col2;

    if (len0 != 0.0f)
        col0 = col0 / len0;
    if (len1 != 0.0f)
        col1 = col1 / len1;
    if (len2 != 0.0f)
        col2 = col2 / len2;

    float m00 = col0[0], m10 = col0[1], m20 = col0[2];
    float m01 = col1[0], m11 = col1[1], m21 = col1[2];
    float m02 = col2[0], m12 = col2[1], m22 = col2[2];

    float trace = m00 + m11 + m22;
    float qx, qy, qz, qw;

    if (trace > 0.0f)
    {
        float s = sqrtf(trace + 1.0f) * 2.0f;
        qw = 0.25f * s;
        qx = (m21 - m12) / s;
        qy = (m02 - m20) / s;
        qz = (m10 - m01) / s;
    }
    else if (m00 > m11 && m00 > m22)
    {
        float s = sqrtf(1.0f + m00 - m11 - m22) * 2.0f;
        qw = (m21 - m12) / s;
        qx = 0.25f * s;
        qy = (m01 + m10) / s;
        qz = (m02 + m20) / s;
    }
    else if (m11 > m22)
    {
        float s = sqrtf(1.0f + m11 - m00 - m22) * 2.0f;
        qw = (m02 - m20) / s;
        qx = (m01 + m10) / s;
        qy = 0.25f * s;
        qz = (m12 + m21) / s;
    }
    else
    {
        float s = sqrtf(1.0f + m22 - m00 - m11) * 2.0f;
        qw = (m10 - m01) / s;
        qx = (m02 + m20) / s;
        qy = (m12 + m21) / s;
        qz = 0.25f * s;
    }

    return MVector4f(qx, qy, qz, qw);
}

static void SetTranslation(MMatrix4f &mat, const MVector3f &v)
{
    mat.m[0][3] = v[0];
    mat.m[1][3] = v[1];
    mat.m[2][3] = v[2];
    mat.m[3][3] = 1.0f; // на случай неединичной нижней строки
}

static void SetScale(MMatrix4f &mat, const MVector3f &v)
{
    // Нормализуем столбцы и масштабируем
    MVector3f col0 = MVector3f(mat.m[0][0], mat.m[1][0], mat.m[2][0]).Normalize() * v[0];
    MVector3f col1 = MVector3f(mat.m[0][1], mat.m[1][1], mat.m[2][1]).Normalize() * v[1];
    MVector3f col2 = MVector3f(mat.m[0][2], mat.m[1][2], mat.m[2][2]).Normalize() * v[2];

    mat.m[0][0] = col0[0];
    mat.m[1][0] = col0[1];
    mat.m[2][0] = col0[2];
    mat.m[0][1] = col1[0];
    mat.m[1][1] = col1[1];
    mat.m[2][1] = col1[2];
    mat.m[0][2] = col2[0];
    mat.m[1][2] = col2[1];
    mat.m[2][2] = col2[2];
}

static void SetRotation(MMatrix4f &mat, const MVector4f &quat)
{
    // Сохраняем текущий масштаб и трансляцию
    MVector3f scale = GetScale(mat);
    MVector3f trans = GetTranslation(mat);

    MVector4f q = quat.GetNormalized();
    float xx = q[0] * q[0];
    float yy = q[1] * q[1];
    float zz = q[2] * q[2];
    float xy = q[0] * q[1];
    float xz = q[0] * q[2];
    float yz = q[1] * q[2];
    float wx = q[3] * q[0];
    float wy = q[3] * q[1];
    float wz = q[3] * q[2];

    // Чистая матрица поворота (столбцы = повёрнутые оси)
    mat.m[0][0] = 1.0f - 2.0f * (yy + zz);
    mat.m[1][0] = 2.0f * (xy + wz);
    mat.m[2][0] = 2.0f * (xz - wy);

    mat.m[0][1] = 2.0f * (xy - wz);
    mat.m[1][1] = 1.0f - 2.0f * (xx + zz);
    mat.m[2][1] = 2.0f * (yz + wx);

    mat.m[0][2] = 2.0f * (xz + wy);
    mat.m[1][2] = 2.0f * (yz - wx);
    mat.m[2][2] = 1.0f - 2.0f * (xx + yy);

    // Восстанавливаем масштаб и трансляцию
    SetScale(mat, scale);
    SetTranslation(mat, trans);
}

static void SetRotation(MMatrix4f &mat, const MVector3f &euler)
{
    MVector3f scale = GetScale(mat);
    MVector3f trans = GetTranslation(mat);

    float radX = euler[0] * (PI / 180.0);
    float radY = euler[1] * (PI / 180.0);
    float radZ = euler[2] * (PI / 180.0);

    float cosX = cosf(radX), sinX = sinf(radX);
    float cosY = cosf(radY), sinY = sinf(radY);
    float cosZ = cosf(radZ), sinZ = sinf(radZ);

    float cosYcosZ = cosY * cosZ;
    float cosYsinZ = cosY * sinZ;
    float sinYcosZ = sinY * cosZ;
    float sinYsinZ = sinY * sinZ;

    // Порядок ZYX (сначала вокруг X, потом Y, потом Z) для column‑major
    mat.m[0][0] = cosYcosZ;
    mat.m[1][0] = cosYsinZ;
    mat.m[2][0] = -sinY;

    mat.m[0][1] = sinX * sinYcosZ - cosX * sinZ;
    mat.m[1][1] = sinX * sinYsinZ + cosX * cosZ;
    mat.m[2][1] = sinX * cosY;

    mat.m[0][2] = cosX * sinYcosZ + sinX * sinZ;
    mat.m[1][2] = cosX * sinYsinZ - sinX * cosZ;
    mat.m[2][2] = cosX * cosY;

    SetScale(mat, scale);
    SetTranslation(mat, trans);
}

static MMatrix4f CreateTranslationMatrix(const MVector3f &v)
{
    MMatrix4f mat = MMatrix4f::identity();
    mat.m[0][3] = v[0];
    mat.m[1][3] = v[1];
    mat.m[2][3] = v[2];
    return mat;
}

static MMatrix4f CreateScaleMatrix(const MVector3f &v)
{
    MMatrix4f mat = MMatrix4f::identity();
    mat.m[0][0] = v[0];
    mat.m[1][1] = v[1];
    mat.m[2][2] = v[2];
    return mat;
}

static MMatrix4f CreateRotationMatrix(const MVector4f &quat)
{
    MMatrix4f mat = MMatrix4f::identity();
    MVector4f q = quat.GetNormalized();

    float xx = q[0] * q[0];
    float yy = q[1] * q[1];
    float zz = q[2] * q[2];
    float xy = q[0] * q[1];
    float xz = q[0] * q[2];
    float yz = q[1] * q[2];
    float wx = q[3] * q[0];
    float wy = q[3] * q[1];
    float wz = q[3] * q[2];

    mat.m[0][0] = 1.0f - 2.0f * (yy + zz);
    mat.m[1][0] = 2.0f * (xy + wz);
    mat.m[2][0] = 2.0f * (xz - wy);

    mat.m[0][1] = 2.0f * (xy - wz);
    mat.m[1][1] = 1.0f - 2.0f * (xx + zz);
    mat.m[2][1] = 2.0f * (yz + wx);

    mat.m[0][2] = 2.0f * (xz + wy);
    mat.m[1][2] = 2.0f * (yz - wx);
    mat.m[2][2] = 1.0f - 2.0f * (xx + yy);

    return mat;
}

static MMatrix4f CreateRotationMatrix(const MVector3f &euler)
{
    MMatrix4f mat = MMatrix4f::identity();

    float radX = euler[0] * (PI / 180.0);
    float radY = euler[1] * (PI / 180.0);
    float radZ = euler[2] * (PI / 180.0);

    float cosX = cosf(radX), sinX = sinf(radX);
    float cosY = cosf(radY), sinY = sinf(radY);
    float cosZ = cosf(radZ), sinZ = sinf(radZ);

    float cosYcosZ = cosY * cosZ;
    float cosYsinZ = cosY * sinZ;
    float sinYcosZ = sinY * cosZ;
    float sinYsinZ = sinY * sinZ;

    mat.m[0][0] = cosYcosZ;
    mat.m[1][0] = cosYsinZ;
    mat.m[2][0] = -sinY;

    mat.m[0][1] = sinX * sinYcosZ - cosX * sinZ;
    mat.m[1][1] = sinX * sinYsinZ + cosX * cosZ;
    mat.m[2][1] = sinX * cosY;

    mat.m[0][2] = cosX * sinYcosZ + sinX * sinZ;
    mat.m[1][2] = cosX * sinYsinZ - sinX * cosZ;
    mat.m[2][2] = cosX * cosY;

    return mat;
}

static MMatrix4f CreateTransformMatrix(const MVector3f &translation, const MVector4f &rotation, const MVector3f &scale)
{
    return CreateTranslationMatrix(translation) * CreateRotationMatrix(rotation) *
      CreateScaleMatrix(scale); // T * R * S
}

static MMatrix4f CreateTransformMatrix(const MVector3f &translation, const MVector3f &euler, const MVector3f &scale)
{
    return CreateTranslationMatrix(translation) * CreateRotationMatrix(euler) * CreateScaleMatrix(scale);
}

static void AddTranslation(MMatrix4f &mat, const MVector3f &v)
{
    mat = mat * CreateTranslationMatrix(v);
}

static void AddScale(MMatrix4f &mat, const MVector3f &v)
{
    mat = mat * CreateScaleMatrix(v);
}

static void AddRotation(MMatrix4f &mat, const MVector4f &quat)
{
    mat = mat * CreateRotationMatrix(quat);
}

static void AddRotation(MMatrix4f &mat, const MVector3f &euler)
{
    mat = mat * CreateRotationMatrix(euler);
}

static void AddGlobalTranslation(MMatrix4f &mat, const MVector3f &v)
{
    mat = CreateTranslationMatrix(v) * mat;
}

static void AddGlobalRotation(MMatrix4f &mat, const MVector4f &quat)
{
    mat = CreateRotationMatrix(quat) * mat;
}

static void AddGlobalScale(MMatrix4f &mat, const MVector3f &v)
{
    mat = CreateScaleMatrix(v) * mat;
}

template<unsigned int C>
static inline float TableCos(float v)
{
    static float Table[C] {2.0f};
    constexpr float k = float(C) / (PI * 2.0f);
    if(Table[0] > 1.0)
    {
        constexpr float ik = PI * 2.0f / float(C);
        for(unsigned int i = 0; i < C; ++i)
        {
            Table[i] = cosf(ik * i);
        }
    }
    unsigned int c = (v > 0) ? v * k : (-v) * k;
    return Table[c % C];
}

} // namespace wm
