program lpt;

const lptbase : integer = $378;
const hexchars : array[0..15] of char = '0123456789ABCDEF';

procedure writehex(x : integer);
begin
  write(hexchars[x shr 4]);
  write(hexchars[x and $f]);
end;

procedure init;
begin
    Port[lptbase] := $00;
end;

procedure writenybble(x : integer);
begin
    (* set upper nybble and raise signal bit to 1 wait for 1 *)
    Port[lptbase] := $10 or (x and $f);
    (*write('W');*)
    while ((Port[lptbase+1] and $80) = 0) do;
    (*write('a');*)
    Port[lptbase] := Port[lptbase] and $f;
    while ((Port[lptbase+1] and $80) <> 0) do;
    (*write('b');*)
end;

function readnybble : integer;
begin
    (* set upper nybble and raise signal bit to 1 wait for 1 *)
    (*write('R'); *)
    while ((Port[lptbase+1] and $80) = 0) do;
    (*write('a');*)
    readnybble := (Port[lptbase+1] shr 3) and $f;
    Port[lptbase] := Port[lptbase] or $10;

    while ((Port[lptbase+1] and $80) <> 0) do;
    Port[lptbase] := Port[lptbase] and $0f;
    (*write('b');*)
end;

procedure writebyte(ch : integer);
begin
    writenybble(ch shr 4);
    writenybble(ch and $f);
end;

function readbyte : integer;
var
    x : integer;
begin
    x := readnybble shl 4;
    x := x or readnybble;
    readbyte := x;
end;

procedure readbytes(n : integer);
var
    i : integer;
    d : integer;
begin
    for i := 1 to n do begin
        d := readbyte;
        (*writehex(d);*)
    end;
end;

procedure writebytes(n : integer);
var
    i : integer;
    d : integer;
begin
    for i := 1 to n do begin
        writebyte(n);
    end;
end;

var
    data : integer;
    i : integer;
    ch : string[255];
    v1 : integer;
    v2 : integer;
    r : integer;
    rxd : array[0..511] of byte;
begin
    init;
    data := $01;
    writeln(lptbase);
    readln(ch);
    while (ch <> 'q') do begin
        if (ch = 'h') then begin
            writebyte($00);
            v1 := readbyte;
            v2 := readbyte;
            write('Hello: version ');
            write(v1);
            write('.');
            write(v2);
            writeln('');
        end;
        if (ch = 'd') then begin
            writebyte($01);
            readbytes(4);
            writeln('');
        end;
        if (ch = 'D') then begin
            writebyte($02);
            writebyte($01);
            readbytes(4);
            writeln('');
        end;
        if (ch = 'r') then begin
            writebyte($03);
            writebyte($00); (* disk *)
            writebyte($00); (* sector *)
            writebyte($00);
            writebyte($00);
            writebyte($00);
            r := readbyte;
            if (r = 0) then readbytes(512);
            writeln('');
        end;
        if (ch = 'w') then begin
            writebyte($04);
            writebyte($00); (* disk *)
            writebyte($00); (* sector *)
            writebyte($00);
            writebyte($00);
            writebyte($00);
            writebytes(512); (* data *)
            r := readbyte; (* result *)
            if (r <> 0) then write('!');
            writeln('');
        end;
        readln(ch);
    end



end.