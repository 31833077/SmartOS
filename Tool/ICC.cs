using System;
using System.Collections;
using System.Diagnostics;
using System.Reflection;
using System.Text;
using System.Linq;
using System.IO;
using System.Collections.Generic;
using System.Text.RegularExpressions;
using System.Threading;
using System.Xml;
using Microsoft.Win32;
using NewLife.Log;
using NewLife.Web;

//【XScript】C#脚本引擎v1.8源码（2015/2/9更新）
//http://www.newlifex.com/showtopic-369.aspx

namespace NewLife.Reflection
{
    public class Builder
    {
        #region 初始化编译器
        String Complier;
        String Asm;
        String Link;
        String Ar;
        String ObjCopy;
		String IncPath;
        String LibPath;

        public Boolean Init(String basePath = null, Boolean addlib = true)
        {
            if (basePath.IsNullOrEmpty())
            {
                var icc = new ICC();
                if (icc.ToolPath.IsNullOrEmpty() || !Directory.Exists(icc.ToolPath))
                {
                    XTrace.WriteLine("未找到IAR！");
                    return false;
                }

                XTrace.WriteLine("发现 {0} {1} {2}", icc.Name, icc.Version, icc.ToolPath);
                basePath = icc.ToolPath.CombinePath("arm\\bin").GetFullPath();
            }

            Complier = basePath.CombinePath("iccarm.exe").GetFullPath();
            Asm = basePath.CombinePath("iasmarm.exe");
            Link = basePath.CombinePath("ilinkarm.exe");
            Ar = basePath.CombinePath("iarchive.exe");
            ObjCopy = basePath.CombinePath("ielftool.exe");

            IncPath = basePath.CombinePath(@"arm\include").GetFullPath();
            LibPath = basePath.CombinePath(@"arm\lib").GetFullPath();

            // 特殊处理GD32F1x0
            if (GD32) Cortex = Cortex;

            Libs.Clear();
            Objs.Clear();

            // 扫描当前所有目录，作为头文件引用目录
            var ss = new String[] { ".", "..\\SmartOS" };
            foreach (var src in ss)
            {
                var p = src.GetFullPath();
                if (!Directory.Exists(p)) p = ("..\\" + src).GetFullPath();
                if (!Directory.Exists(p)) continue;

                AddIncludes(p, false);
                if (addlib) AddLibs(p);
            }
            ss = new String[] { "..\\Lib", "..\\SmartOSLib", "..\\SmartOSLib\\inc" };
            foreach (var src in ss)
            {
                var p = src.GetFullPath();
                if (!Directory.Exists(p)) p = ("..\\" + src).GetFullPath();
                if (!Directory.Exists(p)) continue;

                AddIncludes(p, false);
                if (addlib) AddLibs(p);
            }
			AddIncludes(IncPath, false);

            return true;
        }
        #endregion

        #region 编译选项
        /// <summary>是否编译调试版。默认true</summary>
        public Boolean Debug { get; set; }

        /// <summary>是否精简版。默认false</summary>
        public Boolean Tiny { get; set; }

        /// <summary>是否仅预处理文件，不编译。默认false</summary>
        public Boolean Preprocess { get; set; }

        private String _CPU = "Cortex-M0";
        /// <summary>处理器。默认M0</summary>
        public String CPU { get; set; }

        private String _Flash = "STM32F0";
        /// <summary>Flash容量</summary>
        public String Flash { get; set; }

        /// <summary>分散加载文件</summary>
        public String Scatter { get; set; }

        private Int32 _Cortex;
        /// <summary>Cortex版本。默认0</summary>
        public Int32 Cortex
        {
            get { return _Cortex; }
            set
            {
                _Cortex = value;
                if (GD32 && value == 0)
                    CPU = "Cortex-M{0}".F(3);
                else
                    CPU = "Cortex-M{0}".F(value);
                if (value == 3)
                    Flash = "STM32F1";
                else
                    Flash = "STM32F{0}".F(value);
                if (value == 4) CPU += ".fp";
            }
        }

        /// <summary>是否GD32芯片</summary>
        public Boolean GD32 { get; set; }

        /// <summary>重新编译时间，默认60分钟</summary>
        public Int32 RebuildTime { get; set; }

        /// <summary>定义集合</summary>
        public ICollection<String> Defines { get; private set; }

        /// <summary>引用头文件路径集合</summary>
        public ICollection<String> Includes { get; private set; }

        /// <summary>源文件集合</summary>
        public ICollection<String> Files { get; private set; }

        /// <summary>对象文件集合</summary>
        public ICollection<String> Objs { get; private set; }

        /// <summary>库文件集合</summary>
        public ICollection<String> Libs { get; private set; }

        /// <summary>扩展编译集合</summary>
        public ICollection<String> ExtBuilds { get; private set; }
        #endregion

        #region 构造函数
		public Builder()
		{
			CPU = "Cortex-M0";
			Flash = "STM32F0";
			RebuildTime = 60;
			
			Defines = new HashSet<String>(StringComparer.OrdinalIgnoreCase);
			Includes = new HashSet<String>(StringComparer.OrdinalIgnoreCase);
			Files = new HashSet<String>(StringComparer.OrdinalIgnoreCase);
			Objs = new HashSet<String>(StringComparer.OrdinalIgnoreCase);
			Libs = new HashSet<String>(StringComparer.OrdinalIgnoreCase);
			ExtBuilds = new HashSet<String>(StringComparer.OrdinalIgnoreCase);
		}
        #endregion

        #region 主要编译方法
        private String _Root;

        public Int32 Compile(String file, Boolean showCmd)
        {
            var objName = GetObjPath(file);

            // 如果文件太新，则不参与编译
            var obj = (objName + ".o").AsFile();
            if (obj.Exists)
            {
                if (RebuildTime > 0 && obj.LastWriteTime > file.AsFile().LastWriteTime)
                {
                    // 单独验证源码文件的修改时间不够，每小时无论如何都编译一次新的
                    if (obj.LastWriteTime.AddMinutes(RebuildTime) > DateTime.Now) return -2;
                }
            }

            obj.DirectoryName.EnsureDirectory(false);

			// --debug --endian=little --cpu=Cortex-M3 --enum_is_int -e --char_is_signed --fpu=None 
			// -Ohz --use_c++_inline 
			// --dlib_config C:\Program Files (x86)\IAR Systems\Embedded Workbench 7.0\arm\INC\c\DLib_Config_Normal.h 
            var sb = new StringBuilder();
			if(file.EndsWithIgnoreCase(".cpp"))
				//sb.Append("--c++ --no_exceptions");
				sb.Append("--eec++");
			else
				sb.Append("--use_c++_inline");
			// -e打开C++扩展
            sb.AppendFormat(" --endian=little --cpu={0} -e --silent", CPU);
			if(Cortex >= 4) sb.Append(" --fpu=None");
			//sb.Append(" --enable_multibytes");
            if (Debug) sb.Append(" --debug");
			// 默认低级优化，发行版-Ohz为代码大小优化，-Ohs为高速度优化
			if (!Debug) sb.Append(" -Ohz");
            foreach (var item in Defines)
            {
                sb.AppendFormat(" -D {0}", item);
            }
            if (Tiny) sb.Append(" -D TINY");
			//var basePath = Complier.CombinePath(@"..\..\..\").GetFullPath();
			//sb.AppendFormat(" --dlib_config \"{0}\\arm\\INC\\c\\DLib_Config_Normal.h\"", basePath);

			if(showCmd)
			{
				Console.Write("命令参数：");
				Console.ForegroundColor = ConsoleColor.Magenta;
				Console.WriteLine(sb);
				Console.ResetColor();
			}
			
			sb.Append(" --diag_suppress  Be006,Pa050,Pa039,Pa089,Pe014,Pe047,Pe068,Pe089,Pe167,Pe177,Pe186,Pe188,Pe375,Pe550,Pe550,Pe223,Pe549,Pe550");

			sb.AppendFormat(" -I.");
            foreach (var item in Includes)
            {
                sb.AppendFormat(" -I{0}", item);
            }

			//if(Preprocess) sb.Append(" --preprocess=cn");
			
            sb.AppendFormat(" -c {0}", file);
            sb.AppendFormat(" -o {0}", Path.GetDirectoryName(objName));

            // 先删除目标文件
            if (obj.Exists) obj.Delete();

            return Complier.Run(sb.ToString(), 100, WriteLog);
        }

        public Int32 Assemble(String file, Boolean showCmd)
        {
            var lstName = GetListPath(file);
            var objName = GetObjPath(file);

            // 如果文件太新，则不参与编译
            var obj = (objName + ".o").AsFile();
            if (obj.Exists)
            {
                if (obj.LastWriteTime > file.AsFile().LastWriteTime)
                {
                    // 单独验证源码文件的修改时间不够，每小时无论如何都编译一次新的
                    if (obj.LastWriteTime.AddHours(1) > DateTime.Now) return -2;
                }
            }

            obj.DirectoryName.EnsureDirectory(false);

			/*
			* -s+ -M<> -w+ -r --cpu Cortex-M3 --fpu None
			* -s+	标记符大小写敏感
			* -r	调试输出
			*/
            var sb = new StringBuilder();
			sb.Append("-s+ -M<> -w+ -S");
            sb.AppendFormat(" --cpu {0}", CPU);
			if(Cortex >= 4) sb.Append(" --fpu=None");
            if (GD32) sb.Append(" -DGD32");
            foreach (var item in Defines)
            {
                sb.AppendFormat(" -D{0}", item);
            }
            if (Debug) sb.Append(" -r");
            if (Tiny) sb.Append(" -DTINY");
			sb.AppendFormat(" -I.");
			
			if(showCmd)
			{
				Console.Write("命令参数：");
				Console.ForegroundColor = ConsoleColor.Magenta;
				Console.WriteLine(sb);
				Console.ResetColor();
			}
			
            foreach (var item in Includes)
            {
                sb.AppendFormat(" -I{0}", item);
            }
			
            sb.AppendFormat(" {0} -O{1}", file, Path.GetDirectoryName(objName));

            // 先删除目标文件
            if (obj.Exists) obj.Delete();

            return Asm.Run(sb.ToString(), 100, WriteLog);
        }

        public Int32 CompileAll()
        {
            Objs.Clear();
            var count = 0;

            // 特殊处理GD32F130
            //if(GD32) Cortex = Cortex;

            // 计算根路径，输出的对象文件以根路径下子路径的方式存放
            var di = Files.First().AsFile().Directory;
            _Root = di.FullName;
            foreach (var item in Files)
            {
                while (!item.StartsWithIgnoreCase(_Root))
                {
                    di = di.Parent;
                    if (di == null) break;

                    _Root = di.FullName;
                }
                if (di == null) break;
            }
            _Root = _Root.EnsureEnd("\\");
            Console.WriteLine("根目录：{0}", _Root);

            // 提前创建临时目录
            var obj = GetObjPath(null);
            var list = new List<String>();

			var cpp = 0;
			var asm = 0;
            foreach (var item in Files)
            {
                var rs = 0;
                var sw = new Stopwatch();
                sw.Start();
                switch (Path.GetExtension(item).ToLower())
                {
                    case ".c":
                    case ".cpp":
                        rs = Compile(item, cpp == 0);
						if(rs != -2) cpp++;
                        break;
                    case ".s":
                        rs = Assemble(item, asm == 0);
						if(rs != -2) asm++;
                        break;
                    default:
                        break;
                }

                sw.Stop();

                if (rs == 0 || rs == -1)
                {
                    Console.Write("编译：{0}\t", item);
                    var old = Console.ForegroundColor;
                    Console.ForegroundColor = ConsoleColor.Green;
                    Console.WriteLine("\t {0:n0}毫秒", sw.ElapsedMilliseconds);
                    Console.ForegroundColor = old;
                }

                if (rs <= 0)
                {
                    if (!Preprocess)
                    {
                        //var fi = obj.CombinePath(Path.GetFileNameWithoutExtension(item) + ".o");
                        var fi = GetObjPath(item) + ".o";
                        list.Add(fi);
                    }
                }
            }

            Console.WriteLine("等待编译完成：");
            var left = Console.CursorLeft;
            var list2 = new List<String>(list);
            var end = DateTime.Now.AddSeconds(10);
            var fs = 0;
            while (fs < Files.Count)
            {
                for (int i = list2.Count - 1; i >= 0; i--)
                {
                    if (File.Exists(list[i]))
                    {
                        fs++;
                        list2.RemoveAt(i);
                    }
                }
                Console.CursorLeft = left;
                Console.Write("\t {0}/{1} = {2:p}", fs, Files.Count, (Double)fs / Files.Count);
                if (DateTime.Now > end)
                {
                    Console.Write(" 等待超时！");
                    break;
                }
                Thread.Sleep(500);
            }
            Console.WriteLine();

            for (int i = 0; i < list.Count; i++)
            {
                if (File.Exists(list[i]))
                {
                    count++;
                    Objs.Add(list[i]);
                }
            }

            return count;
        }

        /// <summary>编译静态库</summary>
        /// <param name="name"></param>
        public void BuildLib(String name = null)
        {
            name = GetOutputName(name);
            XTrace.WriteLine("链接：{0}", name);

            var lib = name.EnsureEnd(".a");
            var sb = new StringBuilder();
            //sb.Append("-static");
            sb.AppendFormat(" --create \"{0}\"", lib);

			Console.Write("命令参数：");
			Console.ForegroundColor = ConsoleColor.Magenta;
			Console.WriteLine(sb);
			Console.ResetColor();

            if (Objs.Count < 6) Console.Write("使用对象文件：");
            foreach (var item in Objs)
            {
                sb.Append(" ");
                sb.Append(item);
                if (Objs.Count < 6) Console.Write(" {0}", item);
            }
            if (Objs.Count < 6) Console.WriteLine();

            var rs = Ar.Run(sb.ToString(), 3000, WriteLog);
            if (name.Contains("\\")) name = name.Substring("\\", "_");
            XTrace.WriteLine("链接完成 {0} {1}", rs, lib);
            if (rs == 0)
                "链接静态库{0}完成".F(name).SpeakAsync();
            else
                "链接静态库{0}失败".F(name).SpeakAsync();
        }

        /// <summary>编译目标文件</summary>
        /// <param name="name"></param>
        /// <returns></returns>
        public Int32 Build(String name = null)
        {
            name = GetOutputName(name);
            Console.WriteLine();
            XTrace.WriteLine("生成：{0}", name);

            /*
             * -o \Exe\application.axf 
             * --redirect _Printf=_PrintfTiny 
             * --redirect _Scanf=_ScanfSmallNoMb 
             * --keep bootloader 
             * --keep gImage2EntryFun0 
             * --keep RAM_IMG2_VALID_PATTEN 
             * --image_input=\ram_1.r.bin,bootloader,LOADER,4
             * --map \List\application.map 
             * --log veneers 
             * --log_file \List\application.log 
             * --config \image2.icf 
             * --diag_suppress Lt009,Lp005,Lp006 
             * --entry Reset_Handler --no_exceptions --no_vfe
			 */
            var lstName = GetListPath(name);
            var objName = GetObjPath(name);

            var sb = new StringBuilder();
            //sb.AppendFormat("--cpu {0} --library_type=microlib --strict", CPU);
			var icf = Scatter;
			if(icf.IsNullOrEmpty()) icf = ".".AsDirectory().GetAllFiles("*.icf", false).FirstOrDefault()?.FullName;
            if (!icf.IsNullOrEmpty() && File.Exists(icf.GetFullPath()))
                sb.AppendFormat(" --config \"{0}\"", icf);
            //else
            //    sb.AppendFormat(" --ro-base 0x08000000 --rw-base 0x20000000 --first __Vectors");
            sb.Append(" --entry Reset_Handler --no_exceptions --no_vfe");

            var axf = objName.EnsureEnd(".axf");
            sb.AppendFormat(" --map \"{0}.map\" -o \"{1}\"", lstName, axf);

			foreach(var item in ExtBuilds)
			{
				sb.AppendFormat(" {0}", item.Trim());
			}
			
			Console.Write("命令参数：");
			Console.ForegroundColor = ConsoleColor.Magenta;
			Console.WriteLine(sb);
			Console.ResetColor();

            if (Objs.Count < 6) Console.Write("使用对象文件：");
            foreach (var item in Objs)
            {
                sb.Append(" ");
                sb.Append(item);
                if (Objs.Count < 6) Console.Write(" {0}", item);
            }
            if (Objs.Count < 6) Console.WriteLine();

            var dic = new Dictionary<String, String>(StringComparer.OrdinalIgnoreCase);
            foreach (var item in Libs)
            {
                var lib = new LibFile(item);
                // 调试版/发行版 优先选用最佳匹配版本
                var old = "";
                // 不包含，直接增加
                if (!dic.TryGetValue(lib.Name, out old))
                {
                    dic.Add(lib.Name, lib.FullName);
                }
                // 已包含，并且新版本更合适，替换
                else
                {
                    Console.WriteLine("{0} Debug={1} Tiny={2}", lib.FullName, lib.Debug, lib.Tiny);
                    var lib2 = new LibFile(old);
                    if (!(lib2.Debug == Debug && lib2.Tiny == Tiny) &&
                    (lib.Debug == Debug && lib.Tiny == Tiny))
                    {
                        dic[lib.Name] = lib.FullName;
                    }
                }
            }

            Console.WriteLine("使用静态库：");
            foreach (var item in dic)
            {
                sb.Append(" ");
                sb.Append(item.Value);
                Console.WriteLine("\t{0}\t{1}", item.Key, item.Value);
            }

            XTrace.WriteLine("链接：{0}", axf);

            //var rs = Link.Run(sb.ToString(), 3000, WriteLog);
            var rs = Link.Run(sb.ToString(), 3000, msg =>
            {
                if (msg.IsNullOrEmpty()) return;

                WriteLog(msg);
            });
            if (rs != 0) return rs;

            // 预处理axf。修改编译信息
            Helper.WriteBuildInfo(axf);

            /*var bin = name.EnsureEnd(".bin");
            XTrace.WriteLine("生成：{0}", bin);
            sb.Clear();
            sb.AppendFormat("--bin \"{0}\" \"{1}\"", axf, bin);
            rs = ObjCopy.Run(sb.ToString(), 3000, WriteLog);*/

            var hex = name.EnsureEnd(".hex");
            XTrace.WriteLine("生成：{0}", hex);
            sb.Clear();
            sb.AppendFormat("--ihex \"{0}\" \"{1}\"", axf, hex);
            rs = ObjCopy.Run(sb.ToString(), 3000, WriteLog);

            if (name.Contains("\\")) name = name.Substring("\\", "_");
            if (rs == 0)
                "编译目标{0}完成".F(name).SpeakAsync();
            else
                "编译目标{0}失败".F(name).SpeakAsync();

            return rs;
        }
        #endregion

        #region 辅助方法
        /// <summary>添加指定目录所有文件</summary>
        /// <param name="path">要编译的目录</param>
        /// <param name="exts">后缀过滤</param>
        /// <param name="excludes">要排除的文件</param>
        /// <returns></returns>
        public Int32 AddFiles(String path, String exts = "*.c;*.cpp", Boolean allSub = true, String excludes = null)
        {
            // 目标目录也加入头文件搜索
            //AddIncludes(path, allSub);

            var count = 0;

            var excs = new HashSet<String>((excludes + "").Split(",", ";"), StringComparer.OrdinalIgnoreCase);

            path = path.GetFullPath().EnsureEnd("\\");
            if (String.IsNullOrEmpty(exts)) exts = "*.c;*.cpp";
            foreach (var item in path.AsDirectory().GetAllFiles(exts, allSub))
            {
                if (!item.Extension.EqualIgnoreCase(".c", ".cpp", ".s")) continue;

                Console.Write("添加：{0}\t", item.FullName);

                var flag = true;
                var ex = "";
                if (excs.Contains(item.Name)) { flag = false; ex = item.Name; }
                if (flag)
                {
                    foreach (var elm in excs)
                    {
                        if (item.Name.Contains(elm)) { flag = false; ex = elm; break; }
                    }
                }
                if (!flag)
                {
                    var old2 = Console.ForegroundColor;
                    Console.ForegroundColor = ConsoleColor.Yellow;
                    Console.WriteLine("\t 跳过 {0}", ex);
                    Console.ForegroundColor = old2;

                    continue;
                }
                Console.WriteLine();

                if (!Files.Contains(item.FullName))
                {
                    count++;
                    Files.Add(item.FullName);
                }
            }

            return count;
        }

        public void AddIncludes(String path, Boolean sub = true, Boolean allSub = true)
        {
            path = path.GetFullPath();
            if (!Directory.Exists(path)) return;

			// 有头文件才要，没有头文件不要
			//var fs = path.AsDirectory().GetAllFiles("*.h;*.hpp");
			//if(!fs.Any()) return;

            //if (!Includes.Contains(path) && (!sub || HasHeaderFile(path)))
			// 有些头文件引用采用目录路径，而不是直接文件名
            if (!Includes.Contains(path))
            {
                WriteLog("引用目录：{0}".F(path));
                Includes.Add(path);
            }

            if (sub)
            {
                var opt = allSub ? SearchOption.AllDirectories : SearchOption.TopDirectoryOnly;
                foreach (var item in path.AsDirectory().GetDirectories("*", opt))
                {
                    if (item.FullName.Contains(".svn")) continue;
                    if (item.Name.EqualIgnoreCase("List", "Obj", "ObjD", "Log")) continue;

                    //if (!Includes.Contains(item.FullName) && HasHeaderFile(item.FullName))
                    if (!Includes.Contains(item.FullName))
                    {
                        WriteLog("引用目录：{0}".F(item.FullName));
                        Includes.Add(item.FullName);
                    }
                }
            }
        }

        Boolean HasHeaderFile(String path)
        {
            return path.AsDirectory().GetFiles("*.h", SearchOption.TopDirectoryOnly).Length > 0;
        }

        public void AddLibs(String path, String filter = null, Boolean allSub = true)
        {
            path = path.GetFullPath();
            if (!Directory.Exists(path)) return;

            if (filter.IsNullOrEmpty()) filter = "*.lib";
            //var opt = allSub ? SearchOption.AllDirectories : SearchOption.TopDirectoryOnly;
            foreach (var item in path.AsDirectory().GetAllFiles(filter, allSub))
            {
                // 不包含，直接增加
                if (!Libs.Contains(item.FullName))
                {
                    var lib = new LibFile(item.FullName);
                    WriteLog("发现静态库：{0, -12} {1}".F(lib.Name, item.FullName));
                    Libs.Add(item.FullName);
                }
            }
        }

        class LibFile
        {
            private String _Name;
            /// <summary>名称</summary>
            public String Name { get { return _Name; } set { _Name = value; } }

            private String _FullName;
            /// <summary>全名</summary>
            public String FullName { get { return _FullName; } set { _FullName = value; } }

            private Boolean _Debug;
            /// <summary>是否调试版文件</summary>
            public Boolean Debug { get { return _Debug; } set { _Debug = value; } }

            private Boolean _Tiny;
            /// <summary>是否精简版文件</summary>
            public Boolean Tiny { get { return _Tiny; } set { _Tiny = value; } }

            public LibFile(String file)
            {
                FullName = file;
                Name = Path.GetFileNameWithoutExtension(file);
                Debug = Name.EndsWithIgnoreCase("D");
                Tiny = Name.EndsWithIgnoreCase("T");
                Name = Name.TrimEnd("D", "T");
            }
        }

        private String GetOutputName(String name)
        {
            if (name.IsNullOrEmpty())
            {
                var file = Environment.GetEnvironmentVariable("XScriptFile");
                if (!file.IsNullOrEmpty())
                {
                    file = Path.GetFileNameWithoutExtension(file);
                    name = file.TrimStart("Build_", "编译_", "Build", "编译").TrimEnd(".cs");
                }
            }
            if (name.IsNullOrEmpty())
                name = ".".GetFullPath().AsDirectory().Name;
            else if (name.StartsWith("_"))
                name = ".".GetFullPath().AsDirectory().Name + name.TrimStart("_");
            if (Tiny)
                name = name.EnsureEnd("T");
            else if (Debug)
                name = name.EnsureEnd("D");

            return name;
        }

        // 输出目录。obj/list等位于该目录下，默认当前目录
        public String Output = "";

        private String GetObjPath(String file)
        {
            var objName = "ICC";
            if (Tiny)
                objName += "T";
            else if (Debug)
                objName += "D";
            objName = Output.CombinePath(objName);
            objName.GetFullPath().EnsureDirectory(false);
            if (!file.IsNullOrEmpty())
            {
                //objName += "\\" + Path.GetFileNameWithoutExtension(file);
                var p = file.IndexOf(_Root, StringComparison.OrdinalIgnoreCase);
                if (p == 0) file = file.Substring(_Root.Length);

                objName = objName.CombinePath(file);
                p = objName.LastIndexOf('.');
                if (p > 0) objName = objName.Substring(0, p);
            }

            return objName;
        }

        private String GetListPath(String file)
        {
            var lstName = "List";
            lstName = Output.CombinePath(lstName);
            lstName.GetFullPath().EnsureDirectory(false);
            if (!file.IsNullOrEmpty())
                lstName += "\\" + Path.GetFileNameWithoutExtension(file);

            return lstName;
        }
        #endregion

        #region 日志
        void WriteLog(String msg)
        {
            if (msg.IsNullOrEmpty()) return;

            msg = FixWord(msg);
            if (msg.StartsWithIgnoreCase("错误", "Error", "致命错误", "Fatal error") || msg.Contains("Error:"))
                XTrace.Log.Error(msg);
            else
                XTrace.WriteLine(msg);
        }

        private Dictionary<String, String> _Sections = new Dictionary<String, String>();
        /// <summary>片段字典集合</summary>
        public Dictionary<String, String> Sections { get { return _Sections; } set { _Sections = value; } }

        private Dictionary<String, String> _Words = new Dictionary<String, String>(StringComparer.OrdinalIgnoreCase);
        /// <summary>字典集合</summary>
        public Dictionary<String, String> Words { get { return _Words; } set { _Words = value; } }

        public String FixWord(String msg)
        {
            #region 初始化
			var ss = Sections;
            if (ss.Count == 0)
            {
                ss.Add("Fatal error", "致命错误");
                ss.Add("fatal error", "致命错误");
                ss.Add("Could not open file", "无法打开文件");
                ss.Add("No such file or directory", "文件或目录不存在");
                ss.Add("Undefined symbol", "未定义标记");
                ss.Add("referred from", "引用自");
                ss.Add("Program Size", "程序大小");
                ss.Add("Finished", "程序大小");
				ss.Add("In constructor", "构造函数");
				ss.Add("is ambiguous", "含糊不清");
				ss.Add("call of overloaded", "重载调用");
				ss.Add("was not declared in this scope", "在该范围未定义");
				ss.Add("was not declared", "未定义");
				ss.Add("in this scope", "在该范围");
				ss.Add("In function", "在函数");
				ss.Add("In member function", "在成员函数");
				ss.Add("In static member function", "在静态成员函数");
				ss.Add("In destructor", "在析构函数");
				ss.Add("In instantiation of", "在初始化");
				ss.Add("At global scope", "在全局范围");
				ss.Add("if you use", "如果你使用");
				ss.Add("will accept your code", "将接受你的代码");
				ss.Add("but allowing the use of an undeclared name is deprecated", "但是允许使用未声明的名称是过时的");
				ss.Add("there are no arguments to", "没有参数在");
				ss.Add("that depend on a template parameter", "依赖于模板参数");
				ss.Add("so a declaration of", "所以声明");
				ss.Add("must be available", "必须启用");
				ss.Add("initializing argument", "初始参数");
				ss.Add("conflicting declaration", "声明冲突");
				ss.Add("previous declaration", "前一个声明");
				ss.Add("invalid conversion from", "无效转换");
				ss.Add("to pointer type", "到指针类型");
				ss.Add("large integer", "大整数");
				ss.Add("implicitly truncated to", "隐式截断到");
				ss.Add("unsigned type", "无符号类型");
				ss.Add("no matching function for call to", "没有匹配函数去调用");
				ss.Add("cast to pointer from integer of different size", "从不同大小的整数转为指针");
				ss.Add("candidate expects", "候选预期");
				ss.Add("use of deleted function", "函数被删除");
				ss.Add("is implicitly declared as deleted because", "被隐式声明为删除，因为");
				ss.Add("declares a move constructor or move assignment operator", "声明了一个move构造或move赋值运算符");
            }

            if (Words.Count == 0)
            {
                Words.Add("Error", "错误");
                Words.Add("Warning", "警告");
                Words.Add("Warnings", "警告");
                Words.Add("note", "提示");
				Words.Add("candidate", "候选");
				Words.Add("expected", "预期");
				Words.Add("before", "在之前");
                Words.Add("cannot", "不能");
				Words.Add("converting", "转换");
                /*Words.Add("open", "打开");
                Words.Add("source", "源");
                Words.Add("input", "输入");
                Words.Add("file", "文件");
                Words.Add("No", "没有");
                Words.Add("Not", "没有");
                Words.Add("such", "该");
                Words.Add("or", "或");
                Words.Add("And", "与");
                Words.Add("Directory", "目录");
                Words.Add("Enough", "足够");
                Words.Add("Information", "信息");
                Words.Add("to", "去");
                Words.Add("from", "自");
                Words.Add("list", "列出");
                Words.Add("image", "镜像");
                Words.Add("Symbol", "标识");
                Words.Add("Symbols", "标识");
                Words.Add("the", "");
                Words.Add("map", "映射");
                Words.Add("Finished", "完成");
                Words.Add("line", "行");
                Words.Add("messages", "消息");
                Words.Add("this", "这个");
                Words.Add("feature", "功能");
                Words.Add("supported", "被支持");
                Words.Add("on", "在");
                Words.Add("target", "目标");
                Words.Add("architecture", "架构");
                Words.Add("processor", "处理器");
                Words.Add("Undefined", "未定义");
                Words.Add("referred", "引用");*/
            }
            #endregion

            foreach (var item in Sections)
            {
                msg = msg.Replace(item.Key, item.Value);
            }

            //var sb = new StringBuilder();
            //var ss = msg.Trim().Split(" ", ":", "(", ")");
            //var ss = msg.Trim().Split(" ");
            //for (int i = 0; i < ss.Length; i++)
            //{
            //    var rs = "";
            //    if (Words.TryGetValue(ss[i], out rs)) ss[i] = rs;
            //}
            //return String.Join(" ", ss);
            //var ms = Regex.Matches(msg, "");
            msg = Regex.Replace(msg, "(\\w+\\s?)", match =>
            {
                var w = match.Captures[0].Value;
                var rs = "";
                if (Words.TryGetValue(w.Trim(), out rs)) w = rs;
                return w;
            });
            return msg;
        }
        #endregion
    }

    /// <summary>MDK环境</summary>
    public class ICC
    {
        #region 属性
        /// <summary>名称</summary>
        public String Name { get; set; }

        /// <summary>版本</summary>
        public String Version { get; set; }

        /// <summary>工具目录</summary>
        public String ToolPath { get; set; }
        #endregion

        #region 初始化
        /// <summary>初始化</summary>
        public ICC()
        {
			Name = "ICC";

            #region 从注册表获取目录和版本
            if (String.IsNullOrEmpty(ToolPath))
            {
                var reg = Registry.LocalMachine.OpenSubKey(@"Software\IAR Systems\Embedded Workbench\5.0");
                if (reg == null) reg = Registry.LocalMachine.OpenSubKey(@"Software\Wow6432Node\IAR Systems\Embedded Workbench\5.0");
                if (reg != null)
                {
                    ToolPath = reg.GetValue("LastInstallPath") + "";
                    if(ToolPath.Contains(".")) Version = ToolPath.AsDirectory().Name.Split(" ").Last();
                }
            }
            #endregion

            #region 扫描所有根目录，获取MDK安装目录
			foreach (var item in DriveInfo.GetDrives())
			{
				if (!item.IsReady) continue;

				//var p = Path.Combine(item.RootDirectory.FullName, @"Program Files (x86)\IAR Systems\Embedded Workbench 7.0");
				var p = Path.Combine(item.RootDirectory.FullName, @"Program Files\IAR Systems");
				if (!Directory.Exists(p)) p = Path.Combine(item.RootDirectory.FullName, @"Program Files (x86)\IAR Systems");
				if (Directory.Exists(p))
				{
					var f = p.AsDirectory().GetAllFiles("iccarm.exe", true).LastOrDefault();
					if(f != null)
					{
						ToolPath = f.Directory.FullName.CombinePath(@"..\..\").GetFullPath();
						if(ToolPath.Contains(".")) Version = ToolPath.AsDirectory().Name.Split(" ").Last();
						break;
					}
				}
			}
            if (String.IsNullOrEmpty(ToolPath)) throw new Exception("无法获取ICC安装目录！");
            #endregion
        }
        #endregion
    }

    /*
     * 脚本功能：
     * 1，向固件写入编译时间、编译机器等编译信息
     * 2，生成bin文件，显示固件大小
     * 3，直接双击使用时，设置项目编译后脚本为本脚本
     * 4，清理无用垃圾文件
     * 5，从SmartOS目录更新脚本自己
     */

    public class Helper
    {
        static Int32 Main2(String[] args)
        {
            // 双击启动有两个参数，第一个是脚本自身，第二个是/NoLogo
            if (args == null || args.Length <= 2)
            {
                SetProjectAfterMake();
            }
            else
            {
                var axf = GetAxf(args);
                if (!String.IsNullOrEmpty(axf))
                {
                    // 修改编译信息
                    if (WriteBuildInfo(axf)) MakeBin(axf);
                }
            }

            // 清理垃圾文件
            Clear();

            // 更新脚本自己
            //UpdateSelf();

            // 编译SmartOS
            /*var path = ".".GetFullPath().ToUpper();
            if(path.Contains("STM32F0"))
                "XScript".Run("..\\..\\SmartOS\\Tool\\Build_SmartOS_F0.cs /NoLogo /NoStop");
            else if(path.Contains("STM32F1"))
                "XScript".Run("..\\SmartOS\\Tool\\Build_SmartOS_F1.cs /NoLogo /NoStop");
            else if(path.Contains("STM32F4"))
                "XScript".Run("..\\SmartOS\\Tool\\Build_SmartOS_F4.cs /NoLogo /NoStop");*/

            "完成".SpeakAsync();
            System.Threading.Thread.Sleep(250);

            return 0;
        }

        /// <summary>获取项目文件名</summary>
        /// <returns></returns>
        public static String GetProjectFile()
        {
            var fs = Directory.GetFiles(".".GetFullPath(), "*.uvprojx");
            if (fs.Length == 0) Directory.GetFiles(".".GetFullPath(), "*.uvproj");
            if (fs.Length == 0)
            {
                Console.WriteLine("找不到项目文件！");
                return null;
            }
            if (fs.Length > 1)
            {
                //Console.WriteLine("找到项目文件{0}个，无法定夺采用哪一个！", fs.Length);
                //return null;
                Console.WriteLine("找到项目文件{0}个，选择第一个{1}！", fs.Length, fs[0]);
            }

            return Path.GetFileName(fs[0]);
        }

        /// <summary>设置项目的编译后脚本</summary>
        public static void SetProjectAfterMake()
        {
            Console.WriteLine("设置项目的编译脚本");

            /*
             * 找到项目文件
             * 查找<AfterMake>，开始处理
             * 设置RunUserProg1为1
             * 设置UserProg1Name为XScript.exe Build.cs /NoLogo /NoTime /NoStop
             * 循环查找<AfterMake>，连续处理
             */

            var file = GetProjectFile();
			if(file.IsNullOrEmpty()) return;

            Console.WriteLine("加载项目：{0}", file);
            file = file.GetFullPath();

            var doc = new XmlDocument();
            doc.Load(file);

            var nodes = doc.DocumentElement.SelectNodes("//AfterMake");
            Console.WriteLine("发现{0}个编译目标", nodes.Count);
            var flag = false;
            foreach (XmlNode node in nodes)
            {
                var xn = node.SelectSingleNode("../../../TargetName");
                Console.WriteLine("编译目标：{0}", xn.InnerText);

                xn = node.SelectSingleNode("RunUserProg1");
                xn.InnerText = "1";
                xn = node.SelectSingleNode("UserProg1Name");

                var bat = "XScript.exe Build.cs /NoLogo /NoTime /NoStop /Hide";
                if (xn.InnerText != bat)
                {
                    xn.InnerText = bat;
                    flag = true;
                }
            }

            if (flag)
            {
                Console.WriteLine("保存修改！");
                //doc.Save(file);
                var set = new XmlWriterSettings();
                set.Indent = true;
                // Keil实在烂，XML文件头指明utf-8编码，却不能添加BOM头
                set.Encoding = new UTF8Encoding(false);
                using (var writer = XmlWriter.Create(file, set))
                {
                    doc.Save(writer);
                }
            }
        }

        public static String GetAxf(String[] args)
        {
            var axf = args.FirstOrDefault(e => e.EndsWithIgnoreCase(".axf"));
            if (!String.IsNullOrEmpty(axf)) return axf.GetFullPath();

            // 搜索所有axf文件，找到最新的那一个
            var fis = Directory.GetFiles(".", "*.axf", SearchOption.AllDirectories);
            if (fis != null && fis.Length > 0)
            {
                // 按照修改时间降序的第一个
                return fis.OrderByDescending(e => e.AsFile().LastWriteTime).First().GetFullPath();
            }

            Console.WriteLine("未能从参数中找到输出文件.axf，请在命令行中使用参数#L");
            return null;
        }

        /// <summary>写入编译信息</summary>
        /// <param name="axf"></param>
        /// <returns></returns>
        public static Boolean WriteBuildInfo(String axf)
        {
            // 修改编译时间
            var ft = "yyyy-MM-dd HH:mm:ss";
            var sys = axf.GetFullPath();
            if (!File.Exists(sys)) return false;

            var dt = ft.GetBytes();
            var company = "NewLife_Embedded_Team";
            //var company = "NewLife_Team";
            var name = String.Format("{0}_{1}", Environment.MachineName, Environment.UserName);
            if (name.GetBytes().Length > company.Length)
                name = name.Cut(company.Length);

            var rs = false;
            // 查找时间字符串，写入真实时间
            using (var fs = File.Open(sys, FileMode.Open, FileAccess.ReadWrite))
            {
                if (fs.IndexOf(dt) > 0)
                {
                    fs.Position -= dt.Length;
                    var now = DateTime.Now.ToString(ft);
                    Console.WriteLine("编译时间：{0}", now);
                    fs.Write(now.GetBytes());

                    rs = true;
                }
                fs.Position = 0;
                var ct = company.GetBytes();
                if (fs.IndexOf(ct) > 0)
                {
                    fs.Position -= ct.Length;
                    Console.WriteLine("编译机器：{0}", name);
                    fs.Write(name.GetBytes());
                    // 多写一个0以截断字符串
                    fs.Write((Byte)0);

                    rs = true;
                }
            }

            return rs;
        }

        public static String GetKeil()
        {
            var reg = Registry.LocalMachine.OpenSubKey("Software\\Keil\\Products\\ICC");
            if (reg == null) reg = Registry.LocalMachine.OpenSubKey("Software\\Wow6432Node\\Keil\\Products\\ICC");
            if (reg == null) return null;

            return reg.GetValue("Path") + "";
        }

        /// <summary>生产Bin文件</summary>
        public static void MakeBin(String axf)
        {
            // 修改成功说明axf文件有更新，需要重新生成bin
            // 必须先找到Keil目录，否则没得玩
            var icc = GetKeil();
            if (!String.IsNullOrEmpty(icc) && Directory.Exists(icc))
            {
                var fromelf = icc.CombinePath("ARMCC\\bin\\fromelf.exe");
                //var bin = Path.GetFileNameWithoutExtension(axf) + ".bin";
                var prj = Path.GetFileNameWithoutExtension(GetProjectFile());
                if (Path.GetFileNameWithoutExtension(axf).EndsWithIgnoreCase("D"))
                    prj += "D";
                var bin = prj + ".bin";
                var bin2 = bin.GetFullPath();
                //Process.Start(fromelf, String.Format("--bin {0} -o {1}", axf, bin2));
                var p = new Process();
                p.StartInfo.FileName = fromelf;
                p.StartInfo.Arguments = String.Format("--bin {0} -o {1}", axf, bin2);
                //p.StartInfo.CreateNoWindow = false;
                p.StartInfo.UseShellExecute = false;
                p.Start();
                p.WaitForExit(5000);
                var len = bin2.AsFile().Length;
                Console.WriteLine("生成固件：{0} 共{1:n0}字节/{2:n2}KB", bin, len, (Double)len / 1024);
            }
        }

        /// <summary>清理无用文件</summary>
        public static void Clear()
        {
            // 清理bak
            // 清理dep
            // 清理 用户名后缀
            // 清理txt/ini

            Console.WriteLine();
            Console.WriteLine("清理无用文件");

            var ss = new String[] { "bak", "dep", "txt", "ini", "htm" };
            var list = new List<String>(ss);
            //list.Add(Environment.UserName);

            foreach (var item in list)
            {
                var fs = Directory.GetFiles(".".GetFullPath(), "*." + item);
                if (fs.Length > 0)
                {
                    foreach (var elm in fs)
                    {
                        Console.WriteLine("删除 {0}", elm);
                        try
                        {
                            File.Delete(elm);
                        }
                        catch { }
                    }
                }
            }
        }

        /// <summary>更新脚本自己</summary>
        public static void UpdateSelf()
        {
            var deep = 1;
            // 找到SmartOS目录，里面的脚本可用于覆盖自己
            var di = "../SmartOS".GetFullPath();
            if (!Directory.Exists(di)) { deep++; di = "../../SmartOS".GetFullPath(); }
            if (!Directory.Exists(di)) { deep++; di = "../../../SmartOS".GetFullPath(); }
            if (!Directory.Exists(di)) return;

            var fi = di.CombinePath("Tool/Build.cs");
            switch (deep)
            {
                case 2: fi = di.CombinePath("Tool/Build2.cs"); break;
                case 3: fi = di.CombinePath("Tool/Build3.cs"); break;
                default: break;
            }

            if (!File.Exists(fi)) return;

            var my = "Build.cs".GetFullPath();
            if (my.AsFile().LastWriteTime >= fi.AsFile().LastWriteTime) return;

            try
            {
                File.Copy(fi, my, true);
            }
            catch { }
        }
    }
}