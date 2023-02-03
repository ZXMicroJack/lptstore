program lpt;

const lptbase : integer = $378;
const hexchars : array[0..15] of char = '0123456789ABCDEF';

procedure writehex(x : integer);
begin
  write(hexchars[x shr 4]);
  write(hexchars[x and $f]);
end;

procedure writebyte(x : integer);
begin
    Port[lptbase] := $10 or (x shr 4);
    delay(10);
    Port[lptbase] := x and $f;
    delay(10);
end;

function readbyte : integer;
var
    x : integer;
begin
    while ((Port[lptbase+1] and $80) = 0) do;
    x := (Port[lptbase+1] shr 3) and $f;
    while ((Port[lptbase+1] and $80) <> 0) do;
    x := ((Port[lptbase+1] shl 1) and $f0) or x;
    readbyte := x;
end;


var
    data : integer;
    ch : string[255];
begin
    data := $01;
    writeln(lptbase);
    readln(ch);
    while (ch <> 'q') do begin
        writebyte(data);
        data := data shl 1;
        if (data = $100) then data := $01;
        writehex(Port[lptbase+1] xor 7);
        writeln('');
        readln(ch);
    end



end.