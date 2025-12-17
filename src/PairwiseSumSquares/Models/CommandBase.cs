using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using CommandLine;

namespace PairwiseSumSquares.Models
{
    public class CommandBase
    {
        [Option('l', "lsx", Default = 256, HelpText = "Local size X used for maxGroup on GPU")]
        public int LocalSizeX { get; set; }
    }
}
