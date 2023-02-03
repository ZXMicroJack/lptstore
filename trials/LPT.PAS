program lpt;

const lptbase : integer = $378;

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
        writeln(Port[lptbase+1]);
        readln(ch);
    end



end.