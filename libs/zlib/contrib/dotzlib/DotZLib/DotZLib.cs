//
// © Copyright Henrik Ravn 2004
//
// Use, modification and distribution are subject to the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;


namespace DotZLib
{

    #region Internal types




    internal enum FlushTypes
    {
        None,  Partial,  Sync,  Full,  Finish,  Block
    }

    #region ZStream structure

    [StructLayoutAttribute(LayoutKind.Sequential, Pack=4, Size=0, CharSet=CharSet.Ansi)]
    internal struct ZStream
    {
        public IntPtr next_in;
        public uint avail_in;
        public uint total_in;

        public IntPtr next_out;
        public uint avail_out;
        public uint total_out;

        [MarshalAs(UnmanagedType.LPStr)]
        string msg;
        uint state;

        uint zalloc;
        uint zfree;
        uint opaque;

        int data_type;
        public uint adler;
        uint reserved;
    }

    #endregion

    #endregion

    #region Public enums



    public enum CompressLevel : int
    {



        Default = -1,



        None = 0,



        Best = 9,



        Fastest = 1
    }
    #endregion

    #region Exception classes



    public class ZLibException : ApplicationException
    {






        public ZLibException(int errorCode, string msg) : base(String.Format("ZLib error {0} {1}", errorCode, msg))
        {
        }






        public ZLibException(int errorCode) : base(String.Format("ZLib error {0}", errorCode))
        {
        }
    }
    #endregion

    #region Interfaces




    public interface ChecksumGenerator
    {



        uint Value { get; }




        void Reset();





        void Update(byte[] data);










        void Update(byte[] data, int offset, int count);






        void Update(string data);






        void Update(string data, Encoding encoding);
    }












    public delegate void DataAvailableHandler(byte[] data, int startIndex, int count);




    public interface Codec
    {



        event DataAvailableHandler DataAvailable;






        void Add(byte[] data);








        void Add(byte[] data, int offset, int count);




        void Finish();




        uint Checksum { get; }


    }

    #endregion

    #region Classes



    public class Info
    {
        #region DLL imports
        [DllImport("ZLIB1.dll", CallingConvention=CallingConvention.Cdecl)]
        private static extern uint zlibCompileFlags();

        [DllImport("ZLIB1.dll", CallingConvention=CallingConvention.Cdecl)]
        private static extern string zlibVersion();
        #endregion

        #region Private stuff
        private uint _flags;


        private static int bitSize(uint bits)
        {
            switch (bits)
            {
                case 0: return 16;
                case 1: return 32;
                case 2: return 64;
            }
            return -1;
        }
        #endregion




        public Info()
        {
            _flags = zlibCompileFlags();
        }




        public bool HasDebugInfo { get { return 0 != (_flags & 0x100); } }




        public bool UsesAssemblyCode { get { return 0 != (_flags & 0x200); } }




        public int SizeOfUInt { get { return bitSize(_flags & 3); } }




        public int SizeOfULong { get { return bitSize((_flags >> 2) & 3); } }




        public int SizeOfPointer { get { return bitSize((_flags >> 4) & 3); } }




        public int SizeOfOffset { get { return bitSize((_flags >> 6) & 3); } }




        public static string Version { get { return zlibVersion(); } }
    }

    #endregion

}
