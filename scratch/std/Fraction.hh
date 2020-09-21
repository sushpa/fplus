class Frac {
    Int gcd(Int a, Int b) { return b == 0 ? a : gcd(b, a % b); }
    Int n, d;

    public:
    Frac(Int n, Int d = 1)
        : n(n / gcd(n, d))
        , d(d / gcd(n, d))
    {
    }
    Int num() const { return n; }
    Int den() const { return d; }
    Frac& operator*=(const Frac& rhs)
    {
        Int new_n = n * rhs.n / gcd(n * rhs.n, d * rhs.d);
        d = d * rhs.d / gcd(n * rhs.n, d * rhs.d);
        n = new_n;
        return *this;
    }
};
void print(const Frac& f) { printf("%u/%u", f.num(), f.den()); }
bool operator==(const Frac& lhs, const Frac& rhs)
{
    return lhs.num() == rhs.num() && lhs.den() == rhs.den();
}
bool operator!=(const Frac& lhs, const Frac& rhs) { return !(lhs == rhs); }
Frac operator*(Frac lhs, const Frac& rhs) { return lhs *= rhs; }
