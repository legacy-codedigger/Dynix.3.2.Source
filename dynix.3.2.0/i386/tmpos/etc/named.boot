;
;  boot file for authoritive master name server for COMPANYNAME.COM
;  Note that there should be one primary entry for each SOA record.
;
;

; type    domain				source host/file		backup file

domain		COMPANYNAME.com
primary   	COMPANYNAME.COM			/etc/named.hosts
primary   	0.103.IN-ADDR.ARPA		/etc/named.hosts.rev
primary   	0.0.127.IN-ADDR.ARPA	localhost.rev
