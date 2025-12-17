using System.Diagnostics;
using System.Net;
using System.Runtime.InteropServices;
using CommandLine;
using OpenTK.Graphics.OpenGL;
using OpenTK.Windowing.Common;
using OpenTK.Windowing.Desktop;
using PairwiseSumSquares.Computing;
using PairwiseSumSquares.Models;
using static System.Formats.Asn1.AsnWriter;

namespace PairwiseSumSquares
{
    internal class Program
    {
        static void Main(string[] args)
        {
            OpenGlUtil.CreateContext();
            Console.WriteLine("OpenGL version is " + GL.GetString(StringName.Version));

            var parser = new Parser(c =>
            {
                c.HelpWriter = Console.Out;
                c.AutoHelp = true;
                c.AutoVersion = true;
            });

            var results = parser.ParseArguments<SelfTestCommand>(args);
            results.WithNotParsed(errors =>
            {
                Environment.Exit(1);
            });

            results.WithParsed<SelfTestCommand>(cmd => cmd.Execute());
        }
    }
}
