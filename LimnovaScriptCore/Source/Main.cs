using System;
using System.Runtime.CompilerServices;

namespace Limnova
{

    public class Main
    {
        public Main()
        {
            Native.LogInfo("C#.Limnova.Main.Main()");
            Native.LogTrace("C#.Limnova.Main.Main()");
            Native.LogWarn("C#.Limnova.Main.Main()");
            Native.LogError("C#.Limnova.Main.Main()");

            //Vec3 vec1 = new Vec3(0, 1, 2);
            //InternalCalls.Native_PrintVec3(ref vec1);
            //Vec3 vec2 = new Vec3(0, 1,-2);
            //InternalCalls.Native_PrintVec3(ref vec2);
            //Vec3 v1Xv2 = vec1.Cross(vec2);
            //InternalCalls.Native_PrintVec3(ref v1Xv2);
        }

    }

}
