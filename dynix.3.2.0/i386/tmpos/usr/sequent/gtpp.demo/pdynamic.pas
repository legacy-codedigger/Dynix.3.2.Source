{ use Cartesian coordinates to find the city closest 
   to Beaverton, Oregon, and print the name and 
   distance from Beaverton }

program find_distance ;

const

NCITIES = 10;     { number of cities }
BITE = 1;         { bite of work for a hungry puppy }

type

cityrecord = 
	record
	 name : string [15];	{ names of cities }
	 x : real;		{ x coordinates }
	 y : real		{ y coordinates }
	end; 

var

closest : integer ; { index of nearest city }
shortest : real ;   { distance to nearest city }
cities : array[1..NCITIES] of cityrecord ; { city info }
beaverton : cityrecord ; { coordinates of Beaverton }

procedure m_lock;
	cexternal;
procedure m_unlock;
	cexternal;
procedure m_pfork(procedure a);
	cexternal;
function m_next : longint;
	cexternal;

{ initialize array of city data }

procedure init_cities ;

begin

	cities[1].name := 'CHICAGO';
	cities[1].x := 2000.0;
	cities[1].y := 100.0;
	cities[2].name := 'DENVER';
	cities[2].x := 500.0;
	cities[2].y := -550.0;
	cities[3].name := 'NEW YORK';
	cities[3].x := 1500.0;
	cities[3].y := 100.0;
	cities[4].name := 'SEATTLE';
	cities[4].x := 0.0;
	cities[4].y := 200.0;
	cities[5].name := 'MIAMI';
	cities[5].x := 3500.0;
	cities[5].y := 2000.0;
	cities[6].name := 'SAN FRANCISCO';
	cities[6].x := -100.0;
	cities[6].y := -1000.0;
	cities[7].name := 'RENO';
	cities[7].x := 200.0;
	cities[7].y := -600.0;
	cities[8].name := 'PORTLAND';
	cities[8].x := -17.0;
	cities[8].y := 0.0;
	cities[9].name := 'WASHINGTON D.C';
	cities[9].x := 3000.0;
	cities[9].y := -400.0;
	cities[10].name := 'TILLAMOOK';
	cities[10].x := -70.0;
	cities[10].y := -50.0;

	beaverton.name := 'BEAVERTON';
	beaverton.x := 0.0;
	beaverton.y := 0.0;

end; { of init_cities }

{ find distance to nearest city }
procedure find_dis;
var 

i, base, top : longint ;  { local index, start value, 
				end value }
xsqdis, ysqdis, dist : real ;

begin
	base := BITE * m_next;
	while (base < NCITIES) do
	begin
		top := base + BITE;
		i := base;
		while (i < top) do
		begin
			xsqdis := sqr(beaverton.x - 
				cities[i].x);
			ysqdis := sqr(beaverton.y - 
				cities[i].y);
			dist   := sqrt(xsqdis + ysqdis);
	
			m_lock;
			if (dist < shortest) then
			begin
				closest := i;
				shortest := dist;
			end;
			m_unlock;
	
			i := i + 1 ;
		end;
	base := BITE * m_next;
	end;
end;

begin { main program starts here }

	shortest := 999999999.0;

	init_cities;
	m_pfork(find_dis);
	writeln(cities[closest].name, 
	    ' is closest to Beaverton.');
	writeln(cities[closest].name, ' is ', shortest, 
	    ' miles from Beaverton.');

end.
