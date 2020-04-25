{ multiply two matrices, store results in third 
   matrix, and print results }

program matrix_mul ;

const

SIZE = 9 ;    { (size of matrices)-1 }

type

matrix = array[0..SIZE, 0..SIZE] of real;
integer = longint;

var

a : matrix ;       { first array }
b : matrix ;       { second array }
c : matrix ;       { result array }
nprocs: longint; { number of processes }
ret_val: longint; { return value for m_set_procs }

procedure m_lock;
	cexternal;
procedure m_unlock;
	cexternal;
function m_set_procs(var i : longint) : longint;
	cexternal;
procedure m_pfork(procedure a);
	cexternal;
function m_get_numprocs : longint;
	cexternal;
function m_get_myid : longint;
	cexternal;
procedure m_kill_procs;
	cexternal;

{ initialize matrix function }

procedure init_matrix ;
var
i, j : integer ;
begin
	for i := 0 to SIZE do
	begin
		for j := 0 to SIZE do
		begin
			a[i, j] := (i + j) ;
			b[i, j] := (i - j) ;
		end;
	end;
end; { init_matrix }

{ matrix multiply function }

procedure matmul ;

var

i, j, k : integer; { local loop indices }
nprocs  : integer; { number of processes }

begin
	nprocs := m_get_numprocs;     { number of processes }
	i := m_get_myid;     { start at Nth iteration }
	while (i <= SIZE) do 
	begin
		for j := 0 to SIZE do
		begin
			for k := 0 to SIZE do
				c[i, k] := c[i, k] + a[i, j] * b[j, k];
		end;
		i := i + nprocs;
	end;
end; { matmul}

{ print results procedure }

procedure print_mats ;
var
i, j : integer; { local loop indices }
begin
	for i := 0 to SIZE do
	begin
		for j := 0 to SIZE do
		begin
			writeln('a[',i,',',j,'] = ',a[i,j],  
			   'b[',i,',',j,'] = ',b[i,j],'  c[',i,',',
			   j,'] = ',c[i, j]); 
		end;
	end;
end; {print_mats}

begin { main program starts here}

	writeln('Enter number of processes:'); 
	readln(nprocs);

	init_matrix;         { initialize data arrays }
	ret_val := m_set_procs(nprocs);   { set number of processes }
	m_pfork(matmul);        { do matrix multiply }
	m_kill_procs;        { terminate child processes }
	print_mats;          { print results }

end. { main program }
