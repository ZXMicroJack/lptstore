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
    while ((Port[lptbase+1] and $80) = 0) do;
    Port[lptbase] := x and $f;
    while ((Port[lptbase+1] and $80) <> 0) do;
end;

function readbyte : integer;
var
    x : integer;
begin
    while ((Port[lptbase+1] and $80) = 0) do;
    x := (Port[lptbase+1] shl 1) and $f0;
    Port[lptbase] := Port[lptbase] or $10;
    while ((Port[lptbase+1] and $80) <> 0) do;
    x := ((Port[lptbase+1] shr 3) and $f) or x;
    Port[lptbase] := Port[lptbase] and $e0;
    readbyte := x;
end;


var
    data : integer;
    i : integer;
    ch : string[255];
begin
    data := $01;
    writeln(lptbase);
    readln(ch);
    while (ch <> 'q') do begin
        if (ch = 'r') then begin
            write('Read: ');
            writehex(readbyte);
            writeln('!');
        end;
        if (ch = 'R') then begin
            for i:=1 to 8 do begin
                write('Read: ');
                writehex(readbyte);
                writeln('!');
            end;
        end;
        if (ch = 'w') then begin
            write('Write: ');
            writehex(data);
            writebyte(data);
            data := data shl 1;
            if (data = $100) then data := $01;
            writeln('!');
        end;
        if (ch = 'W') then begin
            for i:=1 to 8 do begin
                write('Write: ');
                writehex(data);
                writebyte(data);
                data := data shl 1;
                if (data = $100) then data := $01;
                writeln('!');
            end;
        end;
        readln(ch);
    end



end.