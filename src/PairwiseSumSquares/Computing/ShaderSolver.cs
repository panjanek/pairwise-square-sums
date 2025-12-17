using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using OpenTK.Graphics.OpenGL;
using PairwiseSumSquares.Models;

namespace PairwiseSumSquares.Computing
{
    public class ShaderSolver
    {
        private ShaderConfig config;

        public int[] results;
        
        private int configUbo;

        private int resultsSsbo;

        private int program;

        public ShaderSolver(int count)
        {
            config.count = count;
            results = new int[count];

            // allocate space for ShaderConfig passed to each compute shader
            configUbo = GL.GenBuffer();
            GL.BindBuffer(BufferTarget.ShaderStorageBuffer, configUbo);
            int configSizeInBytes = Marshal.SizeOf<ShaderConfig>();
            GL.BufferData(BufferTarget.ShaderStorageBuffer, configSizeInBytes, IntPtr.Zero, BufferUsageHint.DynamicDraw);
            GL.BindBufferBase(BufferRangeTarget.ShaderStorageBuffer, 0, configUbo);

            //allocate space for results
            resultsSsbo = GL.GenBuffer();
            GL.BindBuffer(BufferTarget.ShaderStorageBuffer, resultsSsbo);
            GL.BufferData(
                BufferTarget.ShaderStorageBuffer,
                config.count * sizeof(int),
                IntPtr.Zero,
                BufferUsageHint.DynamicRead
            );
            GL.BindBufferBase(BufferRangeTarget.ShaderStorageBuffer, 1, resultsSsbo);

            program = ShaderUtil.CompileAndLinkComputeShader("solver.comp");
        }

        public void Solve(int offset)
        {
            config.offset = offset;
            GL.BindBuffer(BufferTarget.ShaderStorageBuffer, configUbo);
            GL.BufferSubData(
                BufferTarget.ShaderStorageBuffer,
                IntPtr.Zero,
                Marshal.SizeOf<ShaderConfig>(),
                ref config
            );

            GL.UseProgram(program);
            int groups = (config.count + ShaderUtil.LocalSizeX - 1) / ShaderUtil.LocalSizeX;
            GL.DispatchCompute(groups, 1, 1);
            GL.MemoryBarrier(MemoryBarrierFlags.ShaderStorageBarrierBit);


            GL.BindBuffer(BufferTarget.ShaderStorageBuffer, resultsSsbo);
            IntPtr ptr = GL.MapBuffer(BufferTarget.ShaderStorageBuffer, BufferAccess.ReadOnly);
            Marshal.Copy(ptr, results, 0, config.count);
            GL.UnmapBuffer(BufferTarget.ShaderStorageBuffer);
           
            /*
            GL.BindBuffer(BufferTarget.ShaderStorageBuffer, resultsSsbo);
            GL.GetBufferSubData(
                BufferTarget.ShaderStorageBuffer,
                IntPtr.Zero,
                config.count * sizeof(int),
                results
            );*/

            var a = results;
        }


    }
}
