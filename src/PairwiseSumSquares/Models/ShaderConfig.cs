using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace PairwiseSumSquares.Models
{
    [StructLayout(LayoutKind.Sequential)]
    public struct ShaderConfig
    {
        public int offset;
        public int count;
    }
}
