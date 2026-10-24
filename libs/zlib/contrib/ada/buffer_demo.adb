----------------------------------------------------------------
--  ZLib for Ada thick binding.                               --
--                                                            --
--  Copyright (C) 2002-2004 Dmitriy Anisimkov                 --
--                                                            --
--  Open source license information is in the zlib.ads file.  --
----------------------------------------------------------------












with Ada.Streams; use Ada.Streams;
with Ada.Text_IO;

with ZLib; use ZLib;

procedure Buffer_Demo is
   EOL  : Character renames ASCII.LF;
   Text : constant String
     := "Four score and seven years ago our fathers brought forth," & EOL &
        "upon this continent, a new nation, conceived in liberty," & EOL &
        "and dedicated to the proposition that `all men are created equal'.";

   Source : Stream_Element_Array (1 .. Text'Length);
   for Source'Address use Text'Address;

begin
   Ada.Text_IO.Put (Text);
   Ada.Text_IO.New_Line;
   Ada.Text_IO.Put_Line
     ("Uncompressed size : " & Positive'Image (Text'Length) & " bytes");

   declare
      Compressed_Data : Stream_Element_Array (1 .. Text'Length);
      L               : Stream_Element_Offset;
   begin
      Compress : declare
         Compressor : Filter_Type;
         I : Stream_Element_Offset;
      begin
         Deflate_Init (Compressor);



         Translate (Compressor, Source, I, Compressed_Data, L, Finish);
         pragma Assert (I = Source'Last);

         Close (Compressor);

         Ada.Text_IO.Put_Line
           ("Compressed size :   "
            & Stream_Element_Offset'Image (L) & " bytes");
      end Compress;






      Decompress : declare
         Decompressor : Filter_Type;

         Uncompressed_Data : Stream_Element_Array (1 .. Text'Length);

         Block_Size : constant := 4;
         --  This makes sure that the last block contains
         --  only Adler checksum data.

         P : Stream_Element_Offset := Compressed_Data'First - 1;
         O : Stream_Element_Offset;
      begin
         Inflate_Init (Decompressor);

         loop
            Translate
              (Decompressor,
               Compressed_Data
                 (P + 1 .. Stream_Element_Offset'Min (P + Block_Size, L)),
               P,
               Uncompressed_Data
                 (Total_Out (Decompressor) + 1 .. Uncompressed_Data'Last),
               O,
               No_Flush);

               Ada.Text_IO.Put_Line
                 ("Total in : " & Count'Image (Total_In (Decompressor)) &
                  ", out : " & Count'Image (Total_Out (Decompressor)));

               exit when P = L;
         end loop;

         Ada.Text_IO.New_Line;
         Ada.Text_IO.Put_Line
           ("Decompressed text matches original text : "
             & Boolean'Image (Uncompressed_Data = Source));
      end Decompress;
   end;
end Buffer_Demo;
