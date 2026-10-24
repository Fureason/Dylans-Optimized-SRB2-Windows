//
// © Copyright Henrik Ravn 2004
//
// Use, modification and distribution are subject to the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

using System;
using System.IO;
using System.Runtime.InteropServices;

namespace DotZLib
{



	public class GZipStream : Stream, IDisposable
	{
        #region Dll Imports
        [DllImport("ZLIB1.dll", CallingConvention=CallingConvention.Cdecl, CharSet=CharSet.Ansi)]
        private static extern IntPtr gzopen(string name, string mode);

        [DllImport("ZLIB1.dll", CallingConvention=CallingConvention.Cdecl)]
        private static extern int gzclose(IntPtr gzFile);

        [DllImport("ZLIB1.dll", CallingConvention=CallingConvention.Cdecl)]
        private static extern int gzwrite(IntPtr gzFile, int data, int length);

        [DllImport("ZLIB1.dll", CallingConvention=CallingConvention.Cdecl)]
        private static extern int gzread(IntPtr gzFile, int data, int length);

        [DllImport("ZLIB1.dll", CallingConvention=CallingConvention.Cdecl)]
        private static extern int gzgetc(IntPtr gzFile);

        [DllImport("ZLIB1.dll", CallingConvention=CallingConvention.Cdecl)]
        private static extern int gzputc(IntPtr gzFile, int c);

        #endregion

        #region Private data
        private IntPtr _gzFile;
        private bool _isDisposed = false;
        private bool _isWriting;
        #endregion

        #region Constructors






		public GZipStream(string fileName, CompressLevel level)
		{
            _isWriting = true;
            _gzFile = gzopen(fileName, String.Format("wb{0}", (int)level));
            if (_gzFile == IntPtr.Zero)
                throw new ZLibException(-1, "Could not open " + fileName);
		}






        public GZipStream(string fileName)
        {
            _isWriting = false;
            _gzFile = gzopen(fileName, "rb");
            if (_gzFile == IntPtr.Zero)
                throw new ZLibException(-1, "Could not open " + fileName);

        }
        #endregion

        #region Access properties



        public override bool CanRead
        {
            get
            {
                return !_isWriting;
            }
        }





        public override bool CanSeek
        {
            get
            {
                return false;
            }
        }




        public override bool CanWrite
        {
            get
            {
                return _isWriting;
            }
        }
        #endregion

        #region Destructor & IDispose stuff




        ~GZipStream()
        {
            cleanUp(false);
        }




        public void Dispose()
        {
            cleanUp(true);
        }


        private void cleanUp(bool isDisposing)
        {
            if (!_isDisposed)
            {
                gzclose(_gzFile);
                _isDisposed = true;
            }
        }
        #endregion

        #region Basic reading and writing












        public override int Read(byte[] buffer, int offset, int count)
        {
            if (!CanRead) throw new NotSupportedException();
            if (buffer == null) throw new ArgumentNullException();
            if (offset < 0 || count < 0) throw new ArgumentOutOfRangeException();
            if ((offset+count) > buffer.Length) throw new ArgumentException();
            if (_isDisposed) throw new ObjectDisposedException("GZipStream");

            GCHandle h = GCHandle.Alloc(buffer, GCHandleType.Pinned);
            int result;
            try
            {
                result = gzread(_gzFile, h.AddrOfPinnedObject().ToInt32() + offset, count);
                if (result < 0)
                    throw new IOException();
            }
            finally
            {
                h.Free();
            }
            return result;
        }





        public override int ReadByte()
        {
            if (!CanRead) throw new NotSupportedException();
            if (_isDisposed) throw new ObjectDisposedException("GZipStream");
            return gzgetc(_gzFile);
        }












        public override void Write(byte[] buffer, int offset, int count)
        {
            if (!CanWrite) throw new NotSupportedException();
            if (buffer == null) throw new ArgumentNullException();
            if (offset < 0 || count < 0) throw new ArgumentOutOfRangeException();
            if ((offset+count) > buffer.Length) throw new ArgumentException();
            if (_isDisposed) throw new ObjectDisposedException("GZipStream");

            GCHandle h = GCHandle.Alloc(buffer, GCHandleType.Pinned);
            try
            {
                int result = gzwrite(_gzFile, h.AddrOfPinnedObject().ToInt32() + offset, count);
                if (result < 0)
                    throw new IOException();
            }
            finally
            {
                h.Free();
            }
        }







        public override void WriteByte(byte value)
        {
            if (!CanWrite) throw new NotSupportedException();
            if (_isDisposed) throw new ObjectDisposedException("GZipStream");

            int result = gzputc(_gzFile, (int)value);
            if (result < 0)
                throw new IOException();
        }
        #endregion

        #region Position & length stuff





        public override void SetLength(long value)
        {
            throw new NotSupportedException();
        }








        public override long Seek(long offset, SeekOrigin origin)
        {
            throw new NotSupportedException();
        }






        public override void Flush()
        {

        }






        public override long Position
        {
            get
            {
                throw new NotSupportedException();
            }
            set
            {
                throw new NotSupportedException();
            }
        }






        public override long Length
        {
            get
            {
                throw new NotSupportedException();
            }
        }
        #endregion
    }
}
