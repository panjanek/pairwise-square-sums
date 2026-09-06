using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Runtime.ExceptionServices;
using System.Text;
using System.Threading.Tasks;

namespace PerfectSquares
{
    public class Exhaustive
    {
        private ulong maxN = 0;

        private ulong[] squaresList;

        private HashSet<ulong> squaresSet;

        public Exhaustive(ulong maxN)
        {
            this.maxN = maxN;
            this.CreateSquaresIndex();
        }

        private void CreateSquaresIndex()
        {
            ulong max_i = 10 + (ulong)Math.Ceiling(Math.Sqrt(2 * maxN + 10));
            squaresList = new ulong[max_i];
            squaresSet = new HashSet<ulong>();
            for (ulong i =0; i<max_i; i++)
            {
                ulong sq = (i * i);
                squaresList[i] = sq;
                squaresSet.Add(sq);
            }

            var max = squaresList.Max();
            var m = max;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        private ulong FindIndex(ulong sq)
        {
            return (ulong)Math.Ceiling(Math.Sqrt(sq));
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        private bool IsSquare(ulong sq)
        {
            //return squaresSet.Contains(sq);
            //return squaresList[MathUtil.Sqrt(sq)] == sq;
            return squaresList[(int)Math.Floor(Math.Sqrt(sq))] == sq;
        }

        public void SelfTest()
        {
            ulong max = (ulong)Math.Ceiling(Math.Sqrt(this.maxN));
            Console.WriteLine($"Self test max {max}^2 = {maxN}");
            for(ulong i=1; i<=max; i++)
            {
                if (!IsSquare(i * i))
                    Console.WriteLine($"Error for i={i} : {i * i} is square!");
            }

            Console.WriteLine("SelfTest done.");
        }

        public List<ulong[]> Test(ulong a1)
        {
            List<ulong[]> result = new List<ulong[]>();
            ulong s2_i_start = FindIndex(a1);
            ulong s2_i_end = FindIndex(2*a1);
            for (ulong s2_i = s2_i_start; s2_i < s2_i_end; s2_i++)
            {
                ulong s2 = squaresList[s2_i];
                ulong a2 = s2 - a1;
                if (a2 > 0 && a2 < a1)
                {
                    ulong s3_i_start = FindIndex(a2);
                    ulong s3_i_end = FindIndex(2 * a2);
                    for (ulong s3_i = s3_i_start; s3_i < s3_i_end; s3_i++)
                    {
                        ulong s3 = squaresList[s3_i];
                        ulong a3 = s3 - a2;
                        if (a3 > 0 && a3 < a2 && IsSquare(a1 + a3))
                        {
                            //3 found!
                            ulong s4_i_start = FindIndex(a3);
                            ulong s4_i_end = FindIndex(2 * a3);
                            for (ulong s4_i = s4_i_start; s4_i < s4_i_end; s4_i++)
                            {
                                ulong s4 = squaresList[s4_i];
                                ulong a4 = s4 - a3;
                                if (a4 > 0 && a4 < a3 && IsSquare(a1 + a4) && IsSquare(a2 + a4))
                                {
                                    //4 found
                                    //result.Add(new ulong[4] { a4, a3, a2, a1 });

                                    ulong s5_i_start = FindIndex(a4);
                                    ulong s5_i_end = FindIndex(2 * a4);
                                    for (ulong s5_i = s5_i_start; s5_i < s5_i_end; s5_i++)
                                    {
                                        ulong s5 = squaresList[s5_i];
                                        ulong a5 = s5 - a4;
                                        if (a5 > 0 && a5<a4 && IsSquare(a1+a5) && IsSquare(a2+a5) && IsSquare(a3+a5))
                                        {
                                            //5 found
                                            result.Add(new ulong[5] { a5, a4, a3, a2, a1 });
                                            //Console.WriteLine($"found 5 for a1={a1}:    {a5},{a4},{a3},{a2},{a1}");
                                            ulong s6_i_start = FindIndex(a5);
                                            ulong s6_i_end = FindIndex(2 * a5);
                                            for (ulong s6_i = s6_i_start; s6_i < s6_i_end; s6_i++)
                                            {
                                                ulong s6 = squaresList[s6_i];
                                                ulong a6 = s6 - a5;
                                                if (a6 > 0 && a6<a5 && IsSquare(a1+a6) && IsSquare(a2+a6) && IsSquare(a3+a6) && IsSquare(a4+a6))
                                                {
                                                    Console.WriteLine($"\n ----------------------- found 6 for a1={a1}:     [{a6},{a5},{a4},{a3},{a2},{a1}] --------------------------------");
                                                    result.Add(new ulong[6] { a6, a5, a4, a3, a2, a1 });
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
