using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;

namespace PairwiseSumSquares.Computing
{
    public static class ManagedSolver
    {

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        private static ulong FindIndexCeil(ulong sq)
        {
            return (ulong)Math.Ceiling(Math.Sqrt(sq));
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        private static ulong FindIndexFloor(ulong sq)
        {
            return (ulong)Math.Floor(Math.Sqrt(sq));
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        private static bool IsSquare(ulong sq)
        {
            var sqrt = (ulong)Math.Round(Math.Sqrt(sq));
            return sq == sqrt * sqrt;
        }

        public static int GetLargestSetSize(ulong a1)
        {
            int result = 0;
            ulong s2_i_start = FindIndexFloor(a1);
            ulong s2_i_end = FindIndexCeil(2 * a1);
            for (ulong s2_i = s2_i_start; s2_i < s2_i_end; s2_i++)
            {
                ulong s2 = s2_i * s2_i;
                ulong a2 = s2 - a1;
                if (a2 > 0 && a2 < a1)
                {
                    ulong s3_i_start = FindIndexFloor(a2);
                    ulong s3_i_end = FindIndexCeil(2 * a2);
                    for (ulong s3_i = s3_i_start; s3_i < s3_i_end; s3_i++)
                    {
                        ulong s3 = s3_i * s3_i;
                        ulong a3 = s3 - a2;
                        if (a3 > 0 && a3 < a2 && IsSquare(a1 + a3))
                        {
                            ulong s4_i_start = FindIndexFloor(a3);
                            ulong s4_i_end = FindIndexCeil(2 * a3);
                            for (ulong s4_i = s4_i_start; s4_i < s4_i_end; s4_i++)
                            {
                                ulong s4 = s4_i * s4_i;
                                ulong a4 = s4 - a3;
                                if (a4 > 0 && a4 < a3 && IsSquare(a1 + a4) && IsSquare(a2 + a4))
                                {
                                    if (result < 4) 
                                        result = 4;

                                    ulong s5_i_start = FindIndexFloor(a4);
                                    ulong s5_i_end = FindIndexCeil(2 * a4);
                                    for (ulong s5_i = s5_i_start; s5_i < s5_i_end; s5_i++)
                                    {
                                        ulong s5 = s5_i * s5_i;
                                        ulong a5 = s5 - a4;
                                        if (a5 > 0 && a5 < a4 && IsSquare(a1 + a5) && IsSquare(a2 + a5) && IsSquare(a3 + a5))
                                        {
                                            if (result < 5)
                                                result = 5;
         
                                            //Console.WriteLine($"found 5 for a1={a1}:    {a5},{a4},{a3},{a2},{a1}");
                                            ulong s6_i_start = FindIndexFloor(a5);
                                            ulong s6_i_end = FindIndexCeil(2 * a5);
                                            for (ulong s6_i = s6_i_start; s6_i < s6_i_end; s6_i++)
                                            {
                                                ulong s6 = s6_i * s6_i;
                                                ulong a6 = s6 - a5;
                                                if (a6 > 0 && a6 < a5 && IsSquare(a1 + a6) && IsSquare(a2 + a6) && IsSquare(a3 + a6) && IsSquare(a4 + a6))
                                                {
                                                    Console.WriteLine($"\n ----------------------- found 6 for a1={a1}:     [{a6},{a5},{a4},{a3},{a2},{a1}] --------------------------------");
                                                    if (result < 6)
                                                        result = 6;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            return result;
        }

        public static ulong[] GetLargestSet(ulong a1)
        {
            ulong[] result = new ulong[0];
            ulong s2_i_start = FindIndexFloor(a1);
            ulong s2_i_end = FindIndexCeil(2 * a1);
            for (ulong s2_i = s2_i_start; s2_i < s2_i_end; s2_i++)
            {
                ulong s2 = s2_i * s2_i;
                ulong a2 = s2 - a1;
                if (a2 > 0 && a2 < a1)
                {
                    ulong s3_i_start = FindIndexFloor(a2);
                    ulong s3_i_end = FindIndexCeil(2 * a2);
                    for (ulong s3_i = s3_i_start; s3_i < s3_i_end; s3_i++)
                    {
                        ulong s3 = s3_i * s3_i;
                        ulong a3 = s3 - a2;
                        if (a3 > 0 && a3 < a2 && IsSquare(a1 + a3))
                        {
                            //3 found!
                            ulong s4_i_start = FindIndexFloor(a3);
                            ulong s4_i_end = FindIndexCeil(2 * a3);
                            for (ulong s4_i = s4_i_start; s4_i < s4_i_end; s4_i++)
                            {
                                ulong s4 = s4_i * s4_i;
                                ulong a4 = s4 - a3;
                                if (a4 > 0 && a4 < a3 && IsSquare(a1 + a4) && IsSquare(a2 + a4))
                                {
                                    //4 found
                                    result = [a4,a3,a2,a1];

                                    ulong s5_i_start = FindIndexFloor(a4);
                                    ulong s5_i_end = FindIndexCeil(2 * a4);
                                    for (ulong s5_i = s5_i_start; s5_i < s5_i_end; s5_i++)
                                    {
                                        ulong s5 = s5_i * s5_i;
                                        ulong a5 = s5 - a4;
                                        if (a5 > 0 && a5 < a4 && IsSquare(a1 + a5) && IsSquare(a2 + a5) && IsSquare(a3 + a5))
                                        {
                                            if (result.Length < 5)
                                                result = [a5, a4, a3, a2, a1];
                                            ulong s6_i_start = FindIndexFloor(a5);
                                            ulong s6_i_end = FindIndexCeil(2 * a5);
                                            for (ulong s6_i = s6_i_start; s6_i < s6_i_end; s6_i++)
                                            {
                                                ulong s6 = s6_i * s6_i;
                                                ulong a6 = s6 - a5;
                                                if (a6 > 0 && a6 < a5 && IsSquare(a1 + a6) && IsSquare(a2 + a6) && IsSquare(a3 + a6) && IsSquare(a4 + a6))
                                                {
                                                    Console.WriteLine($"\n ----------------------- found 6 for a1={a1}:     [{a6},{a5},{a4},{a3},{a2},{a1}] --------------------------------");
                                                    result = [a6, a5, a4, a3, a2, a1];
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            return result;
        }
    }
}
