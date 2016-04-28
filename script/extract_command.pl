#!/usr/bin/perl

## extract_command.pl
### input: compile_commands.json
### output: build_ir.sh
### output: compiled_files.def

die "Usage: extract_command.pl <compile_commands.json>" unless @ARGV==1;

open WRITE, ">", "./build_ir.sh";
print WRITE "#!";
print WRITE `which bash`;
print WRITE "\n\n\n";
print WRITE "###This bash script is automatically produced by \"extract_command.pl compile_commands.json\", DO NOT CHANGE!###\n\n";

print WRITE "total=0;\n";
print WRITE "succ=0;\n";
print WRITE "check(){\n";
print WRITE "	if [ \$? -eq 0 ]\n";
print WRITE "	then\n";
print WRITE "		succ=`expr \$succ + 1`;\n";
print WRITE "	fi\n";
print WRITE "	total=`expr \$total + 1`;\n";
print WRITE "}\n";

open FILES, ">", "./compiled_files.def";

$filecnt = 0;

open (JSON, $ARGV[0]) or die "cannot open $ARGV[0] for read";
while ($line=<JSON>)
{
	chomp $line;
	@subflds = split(/:/, $line);
	$command = @subflds[0];
	if($command =~ /command/){
		my $compile = @subflds[1];
		@option = split(/\ /, $compile);
		$CC = @option[1]; 
		if($CC =~ /gcc/){$outcommand = "\nclang -g -emit-llvm ";}
		elsif($CC =~ /g\+\+/) {$outcommand = "\nclang++ -g -emit-llvm -I/usr/include/c++/4.2.1 ";}
		elsif($CC =~ /c\+\+/) {$outcommand = "\nclang++ -g -emit-llvm -I/usr/include/c++/4.2.1 ";}
		elsif($CC =~ /cc/) {$outcommand = "\nclang -g -emit-llvm ";}

        
		foreach $field (@option) {
			if($field =~ /^-I/) {
                $outcommand = $outcommand.$field." ";
            }
            
			if($field =~ /^-D/) {				
                $_ = $field;
                s/\\\\\\/\\/;
                s/\\\\\\/\\/;
				s/\(/\\\(/;
				s/\)/\\\)/;
				$field = $_;
				$outcommand = $outcommand.$field." ";
			} 
#			if($field =~ /^-f/) {$outcommand = $outcommand.$field." ";} 
		}
	}
	elsif($command =~ /file/){
		$_ = @subflds[1];
		s/\s\"//;
		s/\"//;	
		$file = $_;
		
		$outcommand = $outcommand."-c ".$_." ";
		s/\.cpp$/\.cpp.bc/;
		s/\.cc$/\.cc.bc/;
		s/\.c$/\.c.bc/;
		$outcommand = $outcommand."-o ".$_;


		if($file ne $lastfile){
		
			if($hash2{$file} == undef){
				$hash2{$file} = 1;	
				$filecnt++;
				print FILES $file."\n";
				print WRITE "\ncd $directory";
		

				print WRITE $outcommand."\n";

				print WRITE "check\n";

				print WRITE "echo \"$filecnt $file to bc\" >&2\n"; 
				$lastfile = $file
			}
		}
	}
	elsif($command =~ /directory/){
		$_ = @subflds[1];
		s/\s\"//;
		s/\"\,//;	
		$directory = $_;
	}
}
close (JSON);

$filecnt = 0;

open (JSON, $ARGV[0]) or die "cannot open $ARGV[0] for read";
while ($line=<JSON>)
{
	chomp $line;
	@subflds = split(/:/, $line);
	$command = @subflds[0];
	if($command =~ /command/) {
		my $compile = @subflds[1];
		@option = split(/\ /, $compile);
		$CC = @option[1]; 
		if($CC =~ /gcc/){$outcommand = "\nclang -g -emit-llvm ";}
		elsif($CC =~ /g\+\+/) {$outcommand = "\nclang++ -g -emit-llvm -I/usr/include/c++/4.2.1 ";}
		elsif($CC =~ /c\+\+/) {$outcommand = "\nclang++ -g -emit-llvm -I/usr/include/c++/4.2.1 ";}
		elsif($CC =~ /cc/) {$outcommand = "\nclang -g -emit-llvm ";}
		foreach $field (@option) {
			if($field =~ /^-I/) {$outcommand = $outcommand.$field." ";}
			if($field =~ /^-D/) {				
				$_ = $field;
                s/\\\\\\/\\/;
                s/\\\\\\/\\/;
                s/\(/\\\(/;
                s/\)/\\\)/;
				$field = $_;
				$outcommand = $outcommand.$field." ";
			} 
#			if($field =~ /^-f/) {$outcommand = $outcommand.$field." ";} 
		}
	}
	elsif($command =~ /file/) {
		$_ = @subflds[1];
		s/\s\"//;
		s/\"//;	
		$file = $_;
		
		$outcommand = $outcommand."-S ".$_." ";
		s/\.cpp$/\.cpp.ll/;
		s/\.cc$/\.cc.ll/;
		s/\.c$/\.c.ll/;
		$outcommand = $outcommand."-o ".$_;

		if($file ne $lastfile){
		
			if($hash1{$file} == undef){
				$hash1{$file} = 1;	
				$filecnt++;
				print WRITE "\ncd $directory";	
	


				print WRITE $outcommand."\n";
				print WRITE "echo \"$filecnt $file to ll\" >&2\n"; 
				$lastfile = $file
			}
		}
	}
	elsif($command =~ /directory/){
		$_ = @subflds[1];
		s/\s\"//;
		s/\"\,//;	
		$directory = $_;
	}
}
close (JSON);

print WRITE "echo \"Total ir \$total\" >&2\n";
print WRITE "echo \"Succ ir \$succ\" >&2\n";

`chmod 777 ./build_ir.sh`
