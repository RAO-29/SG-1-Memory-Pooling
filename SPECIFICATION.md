## CARBON Archive Specification

The following grammar describes the structure of a valid CARBON archive file (in Version 1). You may use 
a [Railroad Diagram Generator](https://www.bottlecaps.de/rr/ui) to generate a syntax diagram.

A CARBON archive is encoded using a marker-based structure:
```
<marker> <data>
```
A `marker` is an particular 8-bit character determining how the byte-stream `data` is interpreted. 
In the following, ``u8``, ``u32``, and ``u64`` refer to a 8-bit, 32-bit resp. 64-bit unsigned integer values.


Using an EBNF notation, the structure of a CARBON file is:

```
archive  ::= archive-header string-table record-header carbon-object baked-indexes
archive-header
         ::= 'MP/CARBON' version record-offset string-id-offset-index-offset
record-header
         ::= 'r' record-header-flags record-size
record-header-flags
         ::= record-header-flags-8-bitmask
record-header-flags-8-bitmask
         ::= read-optimized-flag reserved-bit+
baked-indexes         
         ::= string-id-to-offset?
string-id-to-offset
         ::= '#' key-data-offset value-data-offset table-offset num-entries key-data value-data table-data
key-data
         ::= vector-data                  
value-data
        ::= vector-data
table-data
         ::= vector-data
vector-data
         ::= '|' element-size-32 num-elements-32 cap-elements-32 grow-factor byte+
record-size
         ::= u64
read-optimized-flag
         ::= '1'
           | '0'
reserved-bit
         ::= '1'
           | '0'
string-table
         ::= 'D' num-strings table-flags first-entry-offset extra-field-size ( no-compressor | huffman-compressor )
table-flags
         ::= table-flags-8-bitmask
table-flags-8-bitmask
         ::= not-compressed-flag huffman-compressed-flag reserved-bit+
not-compressed-flag
         ::= '1'
           | '0'
huffman-compressed-flag
         ::= '1'
           | '0'
no-compressor
         ::= uncompressed-string+
string-entry-header  
         ::= '-' next-entry-offset string-id string-length 
uncompressed-string
         ::= string-entry-header character+
huffman-compressor
         ::= huffman-dictionary huffman-string+
huffman-dictionary
         ::= 'd' character prefix-length prefix-code+
huffman-string
         ::= string-entry-header data-length byte+
carbon-object
         ::= '{' object-id object-flags property-offset+ next-object columnified-props+ '}'
columnified-props
         ::= null-prop
           | nullable-prop
           | null-array-prop
           | nullable-array-prop
           | object-array-prop
null-prop
         ::= 'n' column-length key-column
nullable-prop
         ::= ( 'b' | number-type | 't' | 'o' ) column-length key-column offset-column? value-column
number-type
         ::= unsigned-number
           | signed-number
           | 'f'
unsigned-number
         ::= 'r'
           | 'h'
           | 'e'
           | 'g'
signed-number
         ::= 'c'
           | 's'
           | 'i'
           | 'l'
null-array-prop
         ::= 'N' column-length key-column length-column
nullable-array-prop
         ::= ( 'B' | number-array-type | 'T' ) column-length key-column length-column value-column+
number-array-type
         ::= unsigned-number-array
           | signed-number-array
           | 'F'
unsigned-number-array
         ::= 'R'
           | 'H'
           | 'E'
           | 'G'
signed-number-array
         ::= 'C'
           | 'S'
           | 'I'
           | 'L'
object-array-prop
         ::= 'O' column-length key-column offset-column column-groups+
column-groups
         ::= 'X' column-count object-count object-id-column offset-column column+
column   ::= 'x' column-name ( null-column | nullable-column | object-column )
null-column
         ::= 'N' column-length offset-column value-column
nullable-column
         ::= ( 'B' | number-array-type | 'T' ) column-length offset-column positioning-column ( column-length value-column )+
object-column
         ::= 'o' column-length offset-column positioning-column ( column-length carbon-object )+
column-name
         ::= string-id
length-column
         ::= u32+
positioning-column
         ::= u32+
column-count
         ::= u32
object-count 
         ::= u32
object-id-column
         ::= u64
extra-field-size 
         ::= u64         
object-flags
         ::= object-flags-32bitmask
object-flags-32bitmask
         ::= prop-containment-13bitmask prop-array-containment-13bitmask reserved-bit+
prop-containment-13bitmask
         ::= has-null-props-flag has-bool-props-props-flag has-int8-props-flag has-int16-props-flag has-int32-props-flag has-int64-props-flag has-uint8-props-flag has- uint16-props-flag has-uint32-props-flag has-uint64-props-flag has-float-props-flag has-string-props-flag has-object-props-flag
prop-array-containment-13bitmask
         ::= has-null-array-props-flag has-bool-props-array-props-flag has-int8-array-props-flag has-int16-array-props-flag has-int32-array-props-flag has-int64-array-props-flag has-uint8-array-props-flag has- uint16-array-props-flag has-uint32-array-props-flag has-uint64-array-props-flag has-float-array-props-flag has-string-array-props-flag has-object-array-props-flag
property-offset
         ::= u64
column-length
         ::= u32
next-object
         ::= u64
key-column
         ::= string-id+
value-column
         ::= ( nbyte | 'null' )+
nbyte    
         ::= u8+
offset-column
         ::= u64+    
first-entry-offset
         ::= u64   
next-entry-offset
         ::= u64           
version  
         ::= u8
record-offset
         ::= u64
key-data-offset 
         ::= u64
value-data-offset
         ::= u64 
table-offset
         ::= u64
string-id-offset-index-offset
         ::= u64         
prefix-length
         ::= u8
num-strings
         ::= u32
num-entries
         ::= u32
element-size-32 
         ::= u32
num-elements-32 
         ::= u32
cap-elements-32 
         ::= u32       
string-length
         ::= u32
data-length
         ::= u32
string-id
         ::= u64
object-id
         ::= u64
character
         ::= #x0000 - #x00FF
prefix-code
         ::= u8
byte     ::= u8         
```