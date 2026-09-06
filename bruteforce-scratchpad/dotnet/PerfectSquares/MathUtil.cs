using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;

namespace PerfectSquares
{
    public static class MathUtil
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static int BitWidth(ulong x)
        {
            if (x == 0) return 64; // All bits are zero
            int count = 0;
            while ((x & (1UL << 63)) == 0)
            {
                count++;
                x <<= 1;
            }
            return count;
        }

        // implementation for all unsigned integer types
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static ulong Sqrt(ulong n)
        {
            int shift = BitWidth(n);
            shift += shift & 1; // round up to next multiple of 2

            ulong result = 0;
            do
            {
                shift -= 2;
                result <<= 1; // leftshift the result to make the next guess
                result |= 1;  // guess that the next bit is 1
                result ^= (result * result) > (n >> shift) ? 1ul : 0ul; // revert if guess too high
            } while (shift != 0);

            return result;
        }
    }
}
