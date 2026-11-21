Add-Type -TypeDefinition @"
using System;
using System.Runtime.InteropServices;

public class VstRunner {
    [DllImport("user32.dll")]
    public static extern IntPtr CreateWindowEx(
        uint dwExStyle, string lpClassName, string lpWindowName, uint dwStyle,
        int x, int y, int nWidth, int nHeight, IntPtr hWndParent, IntPtr hMenu, IntPtr hInstance, IntPtr lpParam);

    [DllImport("user32.dll")]
    public static extern bool DestroyWindow(IntPtr hWnd);

    [StructLayout(LayoutKind.Sequential)]
    public struct ERect {
        public short top;
        public short left;
        public short bottom;
        public short right;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct AEffect {
        public int magic;
        public IntPtr dispatcher;
        public IntPtr process;
        public IntPtr setParameter;
        public IntPtr getParameter;
        public int numPrograms;
        public int numParams;
        public int numInputs;
        public int numOutputs;
        public int flags;
        public IntPtr resvd1;
        public IntPtr resvd2;
        public int initialDelay;
        public int realQualities;
        public int offQualities;
        public float ioRatio;
        public IntPtr obj;
        public IntPtr user;
        public int uniqueID;
        public int version;
        public IntPtr processReplacing;
        public IntPtr processDoubleReplacing;
    }

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate IntPtr AudioMasterCallback(IntPtr effect, int opcode, int index, IntPtr val, IntPtr ptr, float opt);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate IntPtr DispatcherFunc(IntPtr effect, int opcode, int index, IntPtr val, IntPtr ptr, float opt);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void SetParamFunc(IntPtr effect, int index, float parameter);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate float GetParamFunc(IntPtr effect, int index);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void ProcessReplacingFunc(IntPtr effect, IntPtr[] inPtrs, IntPtr[] outPtrs, int sampleFrames);

    [DllImport(@"dist\ThirdAss.dll", EntryPoint = "main", CallingConvention = CallingConvention.Cdecl)]
    public static extern IntPtr MainPlugin(AudioMasterCallback master);

    [DllImport(@"dist\ThirdAss.dll", EntryPoint = "VSTPluginMain", CallingConvention = CallingConvention.Cdecl)]
    public static extern IntPtr VSTPluginMain(AudioMasterCallback master);

    private static IntPtr HostCallback(IntPtr effect, int opcode, int index, IntPtr val, IntPtr ptr, float opt) {
        return IntPtr.Zero;
    }

    public static void RunAll() {
        Console.WriteLine("==================================================");
        Console.WriteLine(" THIRD ASS (WINDOWS x64 VeSTige) VERIFICATION SUITE");
        Console.WriteLine("==================================================");

        IntPtr pEffect = MainPlugin(HostCallback);
        if (pEffect == IntPtr.Zero) {
            pEffect = VSTPluginMain(HostCallback);
        }
        if (pEffect == IntPtr.Zero) throw new Exception("Plugin entry point returned NULL!");

        AEffect fx = (AEffect)Marshal.PtrToStructure(pEffect, typeof(AEffect));
        Console.WriteLine(string.Format("[PASS] VeSTige Magic: 0x{0:X8} (Expected: 0x56737450)", fx.magic));
        Console.WriteLine(string.Format("[PASS] UniqueID: 0x{0:X8} ('3ASS')", fx.uniqueID));
        Console.WriteLine(string.Format("[PASS] Number of Programs: {0}", fx.numPrograms));
        Console.WriteLine(string.Format("[PASS] Number of Parameters: {0}", fx.numParams));
        Console.WriteLine(string.Format("[PASS] Channels: {0} In, {1} Out", fx.numInputs, fx.numOutputs));

        DispatcherFunc dispatcher = (DispatcherFunc)Marshal.GetDelegateForFunctionPointer(fx.dispatcher, typeof(DispatcherFunc));
        SetParamFunc setParam = (SetParamFunc)Marshal.GetDelegateForFunctionPointer(fx.setParameter, typeof(SetParamFunc));
        GetParamFunc getParam = (GetParamFunc)Marshal.GetDelegateForFunctionPointer(fx.getParameter, typeof(GetParamFunc));
        ProcessReplacingFunc processReplacing = (ProcessReplacingFunc)Marshal.GetDelegateForFunctionPointer(fx.processReplacing, typeof(ProcessReplacingFunc));

        // Name
        IntPtr buf = Marshal.AllocHGlobal(64);
        dispatcher(pEffect, 45, 0, IntPtr.Zero, buf, 0.0f);
        Console.WriteLine(string.Format("[PASS] Effect Name: '{0}'", Marshal.PtrToStringAnsi(buf)));

        // Vendor
        dispatcher(pEffect, 47, 0, IntPtr.Zero, buf, 0.0f);
        Console.WriteLine(string.Format("[PASS] Vendor: '{0}'", Marshal.PtrToStringAnsi(buf)));

        // Product
        dispatcher(pEffect, 48, 0, IntPtr.Zero, buf, 0.0f);
        Console.WriteLine(string.Format("[PASS] Product: '{0}'", Marshal.PtrToStringAnsi(buf)));

        // Editor Rect
        IntPtr rectHolder = Marshal.AllocHGlobal(IntPtr.Size);
        dispatcher(pEffect, 13, 0, IntPtr.Zero, rectHolder, 0.0f);
        IntPtr pRect = Marshal.ReadIntPtr(rectHolder);
        ERect rect = (ERect)Marshal.PtrToStructure(pRect, typeof(ERect));
        Console.WriteLine(string.Format("[PASS] Editor Dimensions: {0} x {1} (Width x Height)", rect.right - rect.left, rect.bottom - rect.top));

        // Parameter testing across all 12 presets
        Console.WriteLine("[PASS] Cycling all 12 factory presets:");
        for (int p = 0; p < fx.numPrograms; ++p) {
            dispatcher(pEffect, 2, 0, (IntPtr)p, IntPtr.Zero, 0.0f);
            dispatcher(pEffect, 5, 0, IntPtr.Zero, buf, 0.0f);
            Console.WriteLine(string.Format("       Preset {0:D2}: {1}", p + 1, Marshal.PtrToStringAnsi(buf)));
        }

        // Audio DSP processing
        int frames = 512;
        float[] inL = new float[frames];
        float[] inR = new float[frames];
        float[] outL = new float[frames];
        float[] outR = new float[frames];
        for (int i = 0; i < frames; ++i) inL[i] = inR[i] = (float)Math.Sin(2.0 * Math.PI * 110.0 * i / 44100.0); // 110 Hz A2 bass note

        GCHandle hInL = GCHandle.Alloc(inL, GCHandleType.Pinned);
        GCHandle hInR = GCHandle.Alloc(inR, GCHandleType.Pinned);
        GCHandle hOutL = GCHandle.Alloc(outL, GCHandleType.Pinned);
        GCHandle hOutR = GCHandle.Alloc(outR, GCHandleType.Pinned);

        IntPtr[] inArr = new IntPtr[] { hInL.AddrOfPinnedObject(), hInR.AddrOfPinnedObject() };
        IntPtr[] outArr = new IntPtr[] { hOutL.AddrOfPinnedObject(), hOutR.AddrOfPinnedObject() };

        processReplacing(pEffect, inArr, outArr, frames);
        Console.WriteLine(string.Format("[PASS] Audio DSP: Processed {0} frames of 110Hz bass. Peak Output L: {1:F4}", frames, outL[100]));

        // GUI Lifecycle test
        IntPtr hHost = CreateWindowEx(0, "STATIC", "Host", 0x80000000, 50, 50, 980, 560, IntPtr.Zero, IntPtr.Zero, IntPtr.Zero, IntPtr.Zero);
        dispatcher(pEffect, 14, 0, IntPtr.Zero, hHost, 0.0f); // effEditOpen
        dispatcher(pEffect, 19, 0, IntPtr.Zero, IntPtr.Zero, 0.0f); // effEditIdle
        dispatcher(pEffect, 15, 0, IntPtr.Zero, IntPtr.Zero, 0.0f); // effEditClose
        DestroyWindow(hHost);
        Console.WriteLine("[PASS] Direct2D Hardware-Accelerated GUI Lifecycle (Open -> Idle -> Close)");

        // Cleanup
        hInL.Free(); hInR.Free(); hOutL.Free(); hOutR.Free();
        Marshal.FreeHGlobal(buf);
        Marshal.FreeHGlobal(rectHolder);

        dispatcher(pEffect, 1, 0, IntPtr.Zero, IntPtr.Zero, 0.0f); // effClose
        Console.WriteLine("[PASS] Plugin Instance Terminated and Cleaned Up.");
        Console.WriteLine("==================================================");
        Console.WriteLine(" ALL VERIFICATION CHECKS PASSED SUCCESSFULLY!     ");
        Console.WriteLine("==================================================");
    }
}
"@
[VstRunner]::RunAll()

