package bmi

import (
    "math/rand"
    "testing"
)

const _size = 65536
var _values, _masks = func()([]uint64, []uint64) {
    rand.Seed(42)
    v := make([]uint64, _size)
    m := make([]uint64, _size)
    for i := 0; i < _size; i++ {
        v[i] = rand.Uint64()
        m[i] = rand.Uint64()
    }
    return v, m
}()

var _result uint64

func BenchmarkGo(b *testing.B) {
    var r uint64
    for i := 0; i < b.N; i++ {
        for j := 0; j < _size; j++ {
            r += ExtractBitsGo(_values[i], _masks[i]);
        }
        _result += r
    }
}

func BenchmarkAsm(b *testing.B) {
    var r uint64
    for i := 0; i < b.N; i++ {
        for j := 0; j < _size; j++ {
            r += _extract_bits_neon(_values[i], _masks[i]);
        }
        _result += r
    }
}
