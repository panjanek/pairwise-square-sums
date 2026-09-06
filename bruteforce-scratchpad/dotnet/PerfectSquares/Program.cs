using System.CommandLine;
using System.Diagnostics;
using System.Diagnostics.CodeAnalysis;
using System.Globalization;
using System.IO;
using System.Net.NetworkInformation;
using System.Net.Sockets;
using System.Runtime.InteropServices;
using System.Text.Json;

namespace PerfectSquares
{
    internal class Program
    {
        static void Main(string[] args)
        {
            //sudo  apt-get update
            //sudo apt-get install -y dotnet-sdk-8.0

            //https://github.com/NVIDIA/cuda-samples

            //c6in.metal

            /*
            for(ulong x=0; x<1000000000; x++)
            {
                var s = MathUtil.Sqrt(x);
                var e = (ulong)Math.Floor(Math.Sqrt(x));
                if (s != e)
                    Console.WriteLine($"ERROR: {s} {e}");
            }*/

            var startOption = new Option<ulong>("--start", () => 1, "start searching from number");
            var endOption = new Option<ulong>("--end", () => 10000000000000000, "search until");
            var batchOption = new Option<ulong>("--batch", () => 1000000, "batch size");
            var threadsOption = new Option<int>("--threads", () => 8, "number of pararell threads");
            var outOption = new Option<string>("--out", "result file name") { IsRequired = true };
            var typeOption = new Option<string>("--type", () => "exhaustive", "search type") { IsRequired = true,  }.FromAmong("exhaustive", "heuristics");
            var rootCommand = new RootCommand("Search for sets of numbers pairwise summing to perfect squares - https://math.stackexchange.com/questions/1576986/pairwise-sums-are-perfect-squares")
            {
                startOption,endOption,batchOption,threadsOption,outOption, typeOption
            };


            rootCommand.SetHandler((ulong start, ulong end, ulong batch, int threads, string fn, string type) =>
            {
                if (type == "exhaustive")
                {
                    ProcessExhaustive(start, end, batch, threads, fn);
                }
                else
                {
                    ProcessHeuristics();
                }
            }, startOption, endOption, batchOption, threadsOption, outOption, typeOption);
            rootCommand.Invoke(args);
        }

        private static void ProcessExhaustive(ulong start, ulong end, ulong batch, int threads, string fn)
        {
            Console.WriteLine($"Start exhaustive search on {threads} threads, batch {batch}, results will be saved to {fn}");
            var exh = new Exhaustive(end);
            //exh.SelfTest();

            ulong batchStart = start;
            ulong batchEnd = start + batch;
            var dot = batch / 50;
            if (dot <= 1)
                dot = 1;
            List<ulong[]> total = new List<ulong[]>();
            Stopwatch totalStopwatch = new Stopwatch();
            totalStopwatch.Start();
            while (batchStart<end)
            {
                Stopwatch stopwatch = new Stopwatch();
                stopwatch.Start();
                
                Console.WriteLine($"{batchStart}..{batchEnd}");
                Parallel.For((long)batchStart, (long)batchEnd, new ParallelOptions { MaxDegreeOfParallelism = threads }, x =>
                {
                    ulong a1 = (ulong)x;

                    if (a1 % dot == 0)
                    {
                        Console.Write('.');
                    }

                    var res = exh.Test(a1);
                    if (res.Count > 0)
                    {
                        lock (exh)
                        {
                            if (res.Count(l => l.Length == 5) > 0)
                                Console.Write("5");
                            if (res.Count(l => l.Length == 6) > 0)
                                Console.Write("6");
                            total.AddRange(res);
                            SaveToFile(fn, total);
                        }
                    }
                });

                stopwatch.Stop();
                Console.WriteLine($"\nBatch processed in {stopwatch.Elapsed.TotalSeconds.ToString("0.00", CultureInfo.InvariantCulture)} seconds   (total processing time {totalStopwatch.Elapsed})");
                Console.WriteLine($"So far found: {total.Count(l => l.Length == 5)} fives and {total.Count(l => l.Length == 6)} sixes");
                SaveToFile(fn, total);

                batchStart = batchEnd;
                batchEnd = batchStart + batch;
            }

            SaveToFile(fn, total);
            Console.WriteLine("done.");
        }

        private static void ProcessHeuristics()
        {
            throw new NotImplementedException();
        }

        private static void SaveToFile(string fn, List<ulong[]> result)
        {
            var model = new SaveModel()
            {
                Fives = result.Where(l => l.Length == 5).ToArray(),
                Sixes = result.Where(l => l.Length == 6).ToArray(),
            };
            var jsonStr = JsonSerializer.Serialize(model, new JsonSerializerOptions() { WriteIndented = true } );

            using (var file = File.CreateText(fn))
            {
                file.Write(jsonStr);
            }
        }
    }

    public class SaveModel
    {
        public ulong[][] Fives { get; set; }

        public ulong[][] Sixes { get; set; }
    }
}
