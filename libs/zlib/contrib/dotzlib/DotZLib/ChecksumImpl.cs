//
// © Copyright Henrik Ravn 2004
//
// Use, modification and distribution are subject to the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

using System;
using System.Runtime.InteropServices;
using System.Text;


namespace DotZLib
{
    #region ChecksumGeneratorBase




    public abstract class ChecksumGeneratorBase : ChecksumGenerator
    {



        protected uint _current;





        public ChecksumGeneratorBase()
        {
            _current = 0;
        }





        public ChecksumGeneratorBase(uint initialValue)
        {
            _current = initialValue;
        }




        public void Reset() { _current = 0; }




        public uint Value { get { return _current; } }












        public abstract void Update(byte[] data, int offset, int count);





        public void Update(byte[] data)
        {
            Update(data, 0, data.Length);
        }






        public void Update(string data)
        {
			Update(Encoding.UTF8.GetBytes(data));
        }






        public void Update(string data, Encoding encoding)
        {
            Update(encoding.GetBytes(data));
        }

    }
    #endregion

    #region CRC32



    public sealed class CRC32Checksum : ChecksumGeneratorBase
    {
        #region DLL imports

        [DllImport("ZLIB1.dll", CallingConvention=CallingConvention.Cdecl)]
        private static extern uint crc32(uint crc, int data, uint length);

        #endregion




        public CRC32Checksum() : base() {}





        public CRC32Checksum(uint initialValue) : base(initialValue) {}










        public override void Update(byte[] data, int offset, int count)
        {
            if (offset < 0 || count < 0) throw new ArgumentOutOfRangeException();
            if ((offset+count) > data.Length) throw new ArgumentException();
            GCHandle hData = GCHandle.Alloc(data, GCHandleType.Pinned);
            try
            {
                _current = crc32(_current, hData.AddrOfPinnedObject().ToInt32()+offset, (uint)count);
            }
            finally
            {
                hData.Free();
            }
        }

    }
    #endregion

    #region Adler



    public sealed class AdlerChecksum : ChecksumGeneratorBase
    {
        #region DLL imports

        [DllImport("ZLIB1.dll", CallingConvention=CallingConvention.Cdecl)]
        private static extern uint adler32(uint adler, int data, uint length);

        #endregion




        public AdlerChecksum() : base() {}





        public AdlerChecksum(uint initialValue) : base(initialValue) {}










        public override void Update(byte[] data, int offset, int count)
        {
            if (offset < 0 || count < 0) throw new ArgumentOutOfRangeException();
            if ((offset+count) > data.Length) throw new ArgumentException();
            GCHandle hData = GCHandle.Alloc(data, GCHandleType.Pinned);
            try
            {
                _current = adler32(_current, hData.AddrOfPinnedObject().ToInt32()+offset, (uint)count);
            }
            finally
            {
                hData.Free();
            }
        }

    }
    #endregion

}