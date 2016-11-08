var build = Builder.Create("GCC");
build.Init();
build.Cortex = 3;
build.Linux = true;
build.RebuildTime = 7 * 24 * 3600;
build.Defines.Add("RTL8710");
build.AddIncludes("..\\..\\..\\GCCLib\\", true, true);
build.AddIncludes("..\\..\\", false);
build.AddFiles(".", "*.c;Sys.cpp;Interrupt.cpp;Time.cpp");
//build.AddFiles("..\\CortexM", "*.c;*.cpp");
build.Libs.Clear();
build.CompileAll();
build.BuildLib("..\\..\\libSmartOS_RTL8710.a");

build.Debug = true;
//build.CompileAll();
//build.BuildLib("..\\..\\libSmartOS_RTL8710.a");