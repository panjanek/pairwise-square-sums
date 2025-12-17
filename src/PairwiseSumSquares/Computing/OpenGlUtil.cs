using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using OpenTK.Windowing.Common;
using OpenTK.Windowing.Desktop;

namespace PairwiseSumSquares.Computing
{
    public static class OpenGlUtil
    {
        public static NativeWindow ContextWindow;
        public static void CreateContext()
        {
            var nativeSettings = new NativeWindowSettings
            {
                Size = new OpenTK.Mathematics.Vector2i(1, 1),
                Title = "Compute",
                Flags = ContextFlags.Offscreen,
                StartVisible = false
            };

            ContextWindow = new NativeWindow(nativeSettings);
            ContextWindow.MakeCurrent();
        }
    }
}
