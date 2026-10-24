//
// © Copyright Henrik Ravn 2004
//
// Use, modification and distribution are subject to the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

using System;
using System.Diagnostics;
using System.Runtime.InteropServices;

namespace DotZLib
{




	public sealed class Deflater : CodecBase
	{
        #region Dll imports
        [DllImport("ZLIB1.dll", CallingConvention=CallingConvention.Cdecl, CharSet=CharSet.Ansi)]
        private static extern int deflateInit_(ref ZStream sz, int level, string vs, int size);

        [DllImport("ZLIB1.dll", CallingConvention=CallingConvention.Cdecl)]
        private static extern int deflate(ref ZStream sz, int flush);

        [DllImport("ZLIB1.dll", CallingConvention=CallingConvention.Cdecl)]
        private static extern int deflateReset(ref ZStream sz);

        [DllImport("ZLIB1.dll", CallingConvention=CallingConvention.Cdecl)]
        private static extern int deflateEnd(ref ZStream sz);
        #endregion





		public Deflater(CompressLevel level) : base()
		{
            int retval = deflateInit_(ref _ztream, (int)level, Info.Version, Marshal.SizeOf(_ztream));
            if (retval != 0)
                throw new ZLibException(retval, "Could not initialize deflater");

            resetOutput();
		}








        public override void Add(byte[] data, int offset, int count)
        {
            if (data == null) throw new ArgumentNullException();
            if (offset < 0 || count < 0) throw new ArgumentOutOfRangeException();
            if ((offset+count) > data.Length) throw new ArgumentException();

            int total = count;
            int inputIndex = offset;
            int err = 0;

            while (err >= 0 && inputIndex < total)
            {
                copyInput(data, inputIndex, Math.Min(total - inputIndex, kBufferSize));
                while (err >= 0 && _ztream.avail_in > 0)
                {
                    err = deflate(ref _ztream, (int)FlushTypes.None);
                    if (err == 0)
                        while (_ztream.avail_out == 0)
                        {
                            OnDataAvailable();
                            err = deflate(ref _ztream, (int)FlushTypes.None);
                        }
                    inputIndex += (int)_ztream.total_in;
                }
            }
            setChecksum( _ztream.adler );
        }





        public override void Finish()
        {
            int err;
            do
            {
                err = deflate(ref _ztream, (int)FlushTypes.Finish);
                OnDataAvailable();
            }
            while (err == 0);
            setChecksum( _ztream.adler );
            deflateReset(ref _ztream);
            resetOutput();
        }




        protected override void CleanUp() { deflateEnd(ref _ztream); }

    }
}
