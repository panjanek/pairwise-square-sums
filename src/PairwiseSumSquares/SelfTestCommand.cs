using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using CommandLine;
using PairwiseSumSquares.Computing;
using PairwiseSumSquares.Models;

namespace PairwiseSumSquares
{
    [Verb("self-test", HelpText = "Perform self test to measure time and compare GPU results with C# results")]
    public class SelfTestCommand : CommandBase
    {
        [Option('s', "start", Default = 750000, HelpText = "Starting number")]
        public int Start { get; set; }

        [Option('c', "count", Default = 50000, HelpText = "How many numbers to check")]
        public int Count { get; set; }

        public void Execute()
        {
            ShaderUtil.LocalSizeX = LocalSizeX;
            SelfTest(Start, Count);
        }

        private void SelfTest(int testOffset, int testCount)
        {
            var stopwatch = new Stopwatch();
            stopwatch.Restart();
            ShaderSolver solver = new ShaderSolver(testCount);
            solver.Solve(testOffset);
            stopwatch.Stop();
            Console.WriteLine($"ShaderSolver tested {testCount} numbers in {stopwatch.Elapsed.TotalSeconds.ToString("0.00")}");

            int[] managed = new int[testCount];
            stopwatch = new Stopwatch();
            stopwatch.Start();
            Parallel.For(0, testCount, i =>
            {
                managed[i] = ManagedSolver.GetLargestSetSize((ulong)(testOffset + i));
            });
            stopwatch.Stop();
            Console.WriteLine($"SelfTest managed: tested {testCount} numbers in {stopwatch.Elapsed.TotalSeconds.ToString("0.00")}");

            for (int i = 0; i < testCount; i++)
            {
                if (managed[i] != solver.results[i])
                    throw new Exception($"Shader solver returned different result for {i} : {solver.results[i]}, should be {managed[i]}");
                if (managed[i] == 5)
                {
                    var list = ManagedSolver.GetLargestSet((ulong)(testOffset + i));
                    string str = string.Join(", ", list);
                    Console.WriteLine("Found five: "+str);
                }
            }
        }
    }
}
