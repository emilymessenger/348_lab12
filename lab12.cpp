/*
 * Lab #12: Extracting double-precision numbers from strings
 *
 * Reads a string, finds and validates a single embedded floating-point number
 * (including scientific notation), ignoring surrounding garbage characters.
 * Returns -999999.99 if no valid number is found or the value is out of range.
 *
 * Key design decisions:
 *   - Integer part accumulated in long long to detect overflow for very large
 *     digit strings (e.g. "99999999999999999999" overflows → INVALID).
 *   - Exponent capped at |308| for double range.
 *   - Trailing digit or '.' after a token marks it invalid.
 *   - No stdlib string-to-number conversion functions used.
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <cctype>
#include <cmath>
#include <climits>

static const double INVALID = -999999.99;

// Raise 10 to an integer power (handles negative exponents)
static double pow10int(int e) {
    double r = 1.0;
    bool neg = e < 0;
    if (neg) e = -e;
    for (int i = 0; i < e; ++i) r *= 10.0;
    return neg ? 1.0 / r : r;
}

// ---------------------------------------------------------------------------
double extractNumeric(const std::string& str) {
    int n = (int)str.size();

    for (int i = 0; i < n; ) {
        char c = str[i];

        // Can this character start a numeric token?
        bool isSign = (c == '+' || c == '-');
        bool isDot  = (c == '.');
        bool isDig  = (isdigit((unsigned char)c) != 0);

        if (isSign) {
            // Sign only starts a number when immediately followed by digit or '.'
            if (i + 1 >= n ||
                (!isdigit((unsigned char)str[i+1]) && str[i+1] != '.')) {
                ++i;
                continue;
            }
        } else if (!isDot && !isDig) {
            ++i;
            continue;
        }

        // --- Attempt to parse a complete candidate starting at index `i` ---
        int pos = i;

        // optional mantissa sign
        bool mantissaNeg = false;
        if (pos < n && (str[pos] == '+' || str[pos] == '-')) {
            mantissaNeg = (str[pos] == '-');
            ++pos;
        }

        // integer part — use long long to detect overflow
        long long intRaw  = 0;
        bool      hasIntD = false;
        bool      intOver = false;
        while (pos < n && isdigit((unsigned char)str[pos])) {
            int d = str[pos] - '0';
            // Check overflow before multiplying
            if (!intOver && intRaw > (LLONG_MAX - d) / 10) {
                intOver = true;
            }
            if (!intOver) intRaw = intRaw * 10 + d;
            hasIntD = true;
            ++pos;
        }

        // decimal point + fractional digits
        double fracVal  = 0.0;
        bool   hasFracD = false;
        if (pos < n && str[pos] == '.') {
            ++pos;
            double place = 0.1;
            while (pos < n && isdigit((unsigned char)str[pos])) {
                fracVal += (str[pos] - '0') * place;
                place   *= 0.1;
                hasFracD = true;
                ++pos;
            }
        }

        // Must have at least one digit in mantissa
        if (!hasIntD && !hasFracD) {
            ++i;   // e.g. standalone '.', '-.', '+.'
            continue;
        }

        // exponent
        bool hasExp = false;
        int  expVal = 0;
        bool expNeg = false;
        bool expBad = false;

        if (pos < n && (str[pos] == 'e' || str[pos] == 'E')) {
            hasExp = true;
            ++pos;

            // optional exponent sign
            if (pos < n && (str[pos] == '+' || str[pos] == '-')) {
                expNeg = (str[pos] == '-');
                ++pos;
                // two consecutive signs → malformed
                if (pos < n && (str[pos] == '+' || str[pos] == '-'))
                    expBad = true;
            }

            // exponent digits
            bool hasExpD = false;
            int  rawExp  = 0;
            while (pos < n && isdigit((unsigned char)str[pos])) {
                if (rawExp <= 500)   // cap to avoid int overflow
                    rawExp = rawExp * 10 + (str[pos] - '0');
                hasExpD = true;
                ++pos;
            }

            if (!hasExpD)                    expBad = true;   // "123e" or "1e+"
            if (pos < n && str[pos] == '.') expBad = true;   // decimal in exponent

            if (!expBad) expVal = expNeg ? -rawExp : rawExp;
        }

        // Trailing digit or '.' immediately after the consumed token → invalid
        bool trailingBad = false;
        if (pos < n) {
            char next = str[pos];
            if (isdigit((unsigned char)next) || next == '.') trailingBad = true;
        }

        if (expBad || trailingBad || intOver) return INVALID;

        // Assemble the final value
        double mantissa = (double)intRaw + fracVal;
        if (mantissaNeg) mantissa = -mantissa;

        double value;
        if (hasExp) {
            if (expVal > 308 || expVal < -308) return INVALID;
            value = mantissa * pow10int(expVal);
        } else {
            value = mantissa;
        }

        if (std::isinf(value) || std::isnan(value)) return INVALID;
        return value;
    }

    return INVALID;
}

// ---------------------------------------------------------------------------
int main() {
    std::string line;
    while (true) {
        std::cout << "Enter a string (or 'END' to quit): ";
        if (!std::getline(std::cin, line)) break;
        if (line == "END") {
            std::cout << "Program terminated." << std::endl;
            break;
        }

        double result = extractNumeric(line);
        if (result == INVALID)
            std::cout << "Invalid input: no valid floating-point number found\n";
        else
            std::cout << "Extracted number: "
                      << std::fixed << std::setprecision(4) << result << "\n";
        std::cout << "\n";
    }
    return 0;
}
