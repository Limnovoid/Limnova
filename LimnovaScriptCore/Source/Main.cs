using System;
using System.Runtime.CompilerServices;

namespace Limnova
{

    public static class InternalCalls
    {
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void NativeLog(string message);
    }

    public class Main
    {
        public float FloatVar { get; set; }

        public Main()
        {
            InternalCalls.NativeLog("Main constructor!");
        }

        public void PrintMessage()
        {
            InternalCalls.NativeLog("Hello World from C#!");
        }

        public void PrintCustomMessage(string message)
        {
            InternalCalls.NativeLog($"C# says: {message}");
        }
    }

}
