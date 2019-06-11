## CARBON Archive Specification (Compact)

The following grammar describes an abstracted version of the CARBON archive specification as written in [SPECIFICATION.md](SPECIFICATION.md).

A CARBON archive is encoded using a marker-based structure:
```
<marker> <data>
```
A `marker` is an particular 8-bit character determining how the byte-stream `data` is interpreted.

Using an EBNF notation, the structure of a CARBON file is:

```
archive               ::= archive-header string-table record-header carbon-object baked-indexes
archive-header        ::= magic-word version-string record-offset string-id-offset-index-offset
record-header         ::= 'r' record-header-flags record-size
baked-indexes         ::= string-id-to-offset?
string-id-to-offset   ::= '#' key-data-off value-data-off table-off num-entries key-vec value-vec table-vec 
string-table          ::= 'D' num-strings table-flags first-entry-offset extra-field-size ( no-compressor | huffman-compressor )
string-entry-header   ::= '-' next-entry-offset string-id string-length 
no-compressor         ::= (string-entry-header character+)+
huffman-compressor    ::= huffman-dictionary huffman-string+
huffman-dictionary    ::= 'd' character prefix-length prefix-code+
huffman-string        ::= string-entry-header data-length byte+
carbon-object         ::= '{' object-id object-flags property-offset+ next-object-offset columnified-props+ '}'
columnified-props     ::= null-prop | nullable-prop | null-array-prop | nullable-array-prop | object-array-prop
null-prop             ::= 'n' column-length key-column
nullable-prop         ::= ( 'b' | number-type | 't' | 'o' ) column-length key-column offset-column? value-column
number-type           ::= unsigned-number | signed-number | 'f'
unsigned-number       ::= 'e' | 'g' | 'r' | 'h' 
signed-number         ::= 'c' | 's' | 'i' | 'l'
null-array-prop       ::= 'N' column-length key-column length-column
nullable-array-prop   ::= ( 'B' | number-array-type | 'T' ) column-length key-column length-column value-column+
number-array-type     ::= unsigned-number-array | signed-number-array | 'F'
unsigned-number-array ::= 'E' | 'G' | 'R' | 'H' 
signed-number-array   ::= 'C' | 'S' | 'I' | 'L'
object-array-prop     ::= 'O' column-length key-column offset-column column-groups+
column-groups         ::= 'X' column-count object-count object-id-column offset-column column+
column                ::= 'x' column-name ( null-column | nullable-column | object-column )
null-column           ::= 'N' column-length offset-column value-column
nullable-column       ::= ( 'B' | number-array-type | 'T' ) column-length offset-column positioning-column ( column-length value-column )+
object-column         ::= 'o' column-length offset-column positioning-column ( column-length carbon-object )+
column-name           ::= string-id
         
```