program lpt;

const lptbase : integer = $378;
const hexchars : array[0..15] of char = '0123456789ABCDEF';

procedure writehex(x : integer);
begin
  write(hexchars[x shr 4]);
  write(hexchars[x and $f]);
end;

function readbyte : integer;
var
    x : integer;
begin
    while ((Port[lptbase+1] and $8) = 0) do;
    x := Port[lptbase+1] and $f0;
    while ((Port[lptbase+1] and $8) <> 0) do;
    x := (Port[lptbase+1] shr 4) or x;
    readbyte := x;
end;


var
    data : integer;
    ch : string[255];
begin
    data := $55;
    writeln(lptbase);
    readln(ch);
    while (ch <> 'q') do begin
        Port[lptbase] := data;
        data := data xor $ff;
        writehex(Port[lptbase]);
        writehex(Port[lptbase+1] xor 7);
        (* writehex(Port[lptbase+2]); *)
        write(' ');
        (* writehex(readbyte); *)
        writeln('');
        readln(ch);
    end



end.