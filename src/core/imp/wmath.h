#pragma once


#include <math.h>
#include <vector>

template<typename T, uint16_t S>
struct MVector
{
    T _data[S];

    MVector<T, S>()
    {
        for (int i = 0; i < S; i++) _data[i] = 0;
    }

    MVector<T, S>(const T* data)
    {
        for (int i = 0; i < S; i++) _data[i] = data[i];
    }

    template<typename... ARGS, typename Check = std::enable_if_t<sizeof...(ARGS) == S && std::conjunction_v<std::is_same<T, ARGS>...>>>
    MVector<T, S>(ARGS... args)
    {
        T data[S]{args...};
        for (int i = 0; i < S; i++) _data[i] = data[i];
    }

    MVector<T, S>(std::initializer_list<T> data)
    {
        int i = 0;
        for (const auto& e : data)
        {
            if (i >= S)
                break;
            _data[i] = e;
            ++i;
        }
        for (; i < S; i++) _data[i] = 0;
    }

    MVector<T, S> operator-() const
    {
        T data[S];
        for (int i = 0; i < S; i++) data[i] = _data[i] * -1;
        return MVector<T, S>(data);
    }

    MVector<T, S> operator+() const
    {
        T data[S];
        for (int i = 0; i < S; i++) data[i] = _data[i];
        return MVector<T, S>(data);
    }

    MVector<T, S> operator+(const MVector<T, S>& other) const
    {
        T data[S];
        for (int i = 0; i < S; i++) data[i] = _data[i] + other._data[i];
        return MVector<T, S>(data);
    }

    MVector<T, S> operator-(const MVector<T, S>& other) const
    {
        T data[S];
        for (int i = 0; i < S; i++) data[i] = _data[i] - other._data[i];
        return MVector<T, S>(data);
    }

    MVector<T, S> operator*(const T& other) const
    {
        T data[S];
        for (int i = 0; i < S; i++) data[i] = _data[i] * other;
        return MVector<T, S>(data);
    }

    T operator*(const MVector<T, S>& other) const
    {
        T data;
        for (int i = 0; i < S; i++) data = _data[i] * other._data[i];
        return data;
    }

    MVector<T, S> operator/(const T& other) const
    {
        T data[S];
        for (int i = 0; i < S; i++) data[i] = _data[i] / other;
        return MVector<T, S>(data);
    }

    T operator!() const
    {
        T ret = _data[0] * _data[0];
        for (int i = 1; i < S; i++) ret += _data[i] * _data[i];
        if constexpr (std::is_same_v<T, float>)
            return sqrtf(ret);
        else if constexpr (std::is_same_v<T, double>)
            return sqrt(ret);
        else
            return (T)(sqrt(ret));
    }

    T& operator[](int i) { return _data[i % S]; }

    const T& operator[](int i) const { return _data[i % S]; }

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
};

typedef  MVector<float, 2> MVector2f;
typedef  MVector<float, 3> MVector3f;
typedef  MVector<float, 4> MVector4f;
typedef  MVector<int, 4> MVector4i;

MVector3f operator/(const MVector3f& a, const MVector3f& b)
{
    return MVector3f({a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]});
}


template<typename T, uint16_t S>
struct MMatrix
{
    T m[S][S] = {0};

    MMatrix<T, S>() = default;

    // Создание единичной матрицы
    static MMatrix<T, S> identity()
    {
        MMatrix<T, S> mat;
        mat.m[0][0] = mat.m[1][1] = mat.m[2][2] = mat.m[3][3] = 1.0f;
        return mat;
    }

    // Умножение матриц
    MMatrix<T, S> operator*(const MMatrix<T, S>& other) const
    {
        MMatrix<T, S> result;
        for (int i = 0; i < S; ++i)
        {
            for (int j = 0; j < S; ++j)
            {
                result.m[i][j] = 0;
                for (int k = 0; k < S; ++k) { result.m[i][j] += m[i][k] * other.m[k][j]; }
            }
        }
        return result;
    }

    // Умножение матрицы на вектор
    MVector<T, S> operator*(const MVector<T, S>& vec) const
    {
        float result[S] = {0};
        for (int i = 0; i < S; ++i)
        {
            for (int j = 0; j < S; ++j) { result[i] += m[i][j] * vec[j]; }
        }
        return MVector<T, S>(result);
    }

    // Преобразование точки (автоматическое добавление w=1)
    MVector<T, S> transformPoint(const MVector<T, S>& point) const
    {
        MVector<T, S> temp = point;
        temp               = *this * temp;
        if (temp[S - 1] != 0)
            return temp / temp[S - 1];
        return temp;
    }

    MVector<T, S> transformVector(const MVector<T, S>& vec) const { return *this * vec; }

    // Создание матрицы переноса
    template<uint16_t V>
    static MMatrix<T, S> translate(const MVector<T, V>& translation)
    {
        static_assert(V <= S, "");
        MMatrix<T, S> mat;
        for (int i = 0; i < S; i++) mat.m[i][i] = 1;

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
    static MMatrix<T, S> scale(const MVector<T, S - 1>& scaling)
    {
        static_assert(V <= S, "");
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


typedef  MMatrix<float, 4> MMatrix4f;
typedef  MMatrix<float, 3> MMatrix3f;
typedef  MMatrix<float, 2> MMatrix2f;
typedef  MMatrix<int, 4> MMatrix4i;

namespace wm
{

static MMatrix4f RotateX(float angle)
{
    float     c   = cos(angle);
    float     s   = sin(angle);
    MMatrix4f mat = MMatrix4f::identity();
    mat.m[1][1]   = c;
    mat.m[1][2]   = -s;
    mat.m[2][1]   = s;
    mat.m[2][2]   = c;
    return mat;
}

static MMatrix4f RotateY(float angle)
{
    float     c   = cos(angle);
    float     s   = sin(angle);
    MMatrix4f mat = MMatrix4f::identity();
    mat.m[0][0]   = c;
    mat.m[0][2]   = s;
    mat.m[2][0]   = -s;
    mat.m[2][2]   = c;
    return mat;
}

static MMatrix4f RotateZ(float angle)
{
    float     c   = cos(angle);
    float     s   = sin(angle);
    MMatrix4f mat = MMatrix4f::identity();
    mat.m[0][0]   = c;
    mat.m[0][1]   = -s;
    mat.m[1][0]   = s;
    mat.m[1][1]   = c;
    return mat;
}

static MMatrix4f Perspective(float fov, float aspect, float _near, float _far)
{
    float f     = 1.0f / tan(fov / 2.0f);
    float range = _near - _far;

    MMatrix4f mat;
    mat.m[0][0] = f / aspect;
    mat.m[1][1] = f;
    mat.m[2][2] = (_far + _near) / range;
    mat.m[2][3] = (2.0f * _far * _near) / range;
    mat.m[3][2] = -1.0f;
    mat.m[3][3] = 0.0f;
    return mat;
}

static MMatrix4f LookAt(const MVector3f& eye, const MVector3f& center, const MVector3f& up)
{
    MVector3f f = (center - eye);
    f           = f / !f;

    MVector3f s = f / up;
    s           = s / !s;

    MVector3f u = s / f;

    MMatrix4f mat = MMatrix4f::identity();
    mat.m[0][0]   = s[0];
    mat.m[0][1]   = s[1];
    mat.m[0][2]   = s[2];

    mat.m[1][0] = u[0];
    mat.m[1][1] = u[1];
    mat.m[1][2] = u[2];

    mat.m[2][0] = -f[0];
    mat.m[2][1] = -f[1];
    mat.m[2][2] = -f[2];

    mat.m[0][3] = -s[0] * eye[0] - s[1] * eye[1] - s[2] * eye[2];
    mat.m[1][3] = -u[0] * eye[0] - u[1] * eye[1] - u[2] * eye[2];
    mat.m[2][3] = f[0] * eye[0] + f[1] * eye[1] + f[2] * eye[2];

    return mat;
}

static MVector3f GetTranslation(const MMatrix4f& mat) {
    return MVector3f();
}

static MVector3f GetScale(const MMatrix4f& mat) {
    return MVector3f();
}

static MVector4f GetRotation(const MMatrix4f& mat) {
    return MVector4f();
}

static void SetTranslation(MMatrix4f& mat, const MVector3f& v) {
  
}

static void SetScale(MMatrix4f& mat, const MVector3f& v) {

}

static void SetRotation(MMatrix4f& mat, const MVector4f& quat) {

}

static void SetRotation(MMatrix4f& mat, const MVector3f& euler) {

}

};