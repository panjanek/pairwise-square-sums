using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Threading.Tasks;
using OpenTK.Graphics.OpenGL;

namespace PairwiseSumSquares.Computing
{
    public static class ShaderUtil
    {
        // 16, 32, 64, 128, 256 - depending on GPU architecture/vendor. Can be set as first commandline parameter
        public static int LocalSizeX = 256;
        public static int CompileAndLinkComputeShader(string compFile)
        {
            // Compile compute shader
            string source = LoadShaderCode(compFile);
            source = source.Replace("{LocalSizeX}", LocalSizeX.ToString());
            Console.WriteLine($"Using LocalSizeX={LocalSizeX}");
            int computeShader = GL.CreateShader(ShaderType.ComputeShader);
            GL.ShaderSource(computeShader, source);
            GL.CompileShader(computeShader);
            GL.GetShader(computeShader, ShaderParameter.CompileStatus, out int status);
            if (status != (int)All.True)
            {
                var log = GL.GetShaderInfoLog(computeShader);
                throw new Exception(log);
            }

            int program = GL.CreateProgram();
            GL.AttachShader(program, computeShader);
            GL.LinkProgram(program);
            GL.GetProgram(program, GetProgramParameterName.LinkStatus, out status);
            if (status != (int)All.True)
            {
                throw new Exception(GL.GetProgramInfoLog(program));
            }

            return program;
        }

        public static string LoadShaderCode(string name)
        {
            var assembly = Assembly.GetExecutingAssembly();
            var a = assembly.GetManifestResourceNames();
            var resourceName = $"PairwiseSumSquares.shaders.{name}";
            using Stream stream = assembly.GetManifestResourceStream(resourceName) ?? throw new InvalidOperationException($"Resource not found: {resourceName}");
            using StreamReader reader = new StreamReader(stream);
            return reader.ReadToEnd();
        }
    }
}
