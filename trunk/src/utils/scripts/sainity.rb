# This file is part of the aMule project.
#
# Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#



# This class represents lines of code, with line-number and text
# It is used to store the source-files once they have been read
# and afterwards to store the lines returned by the filters.
class Line
	def initialize( number, text )
		@number = number
		@text = text
	end

	attr_reader :number
	attr_reader :text
end



class Result
	def initialize( type, file, line = nil )
		@type = type
		@file = file
		@line = line
	end

	def file_name
		@file.slice( /[^\/]+$/ )
	end
	
	def file_path
		@file.slice( /^.*\// )
	end

	attr_reader :type
	attr_reader :file
	attr_reader :line
end



# Base class for Sainity Checkers
#
# This class represents the basic sainity-check, which returns all
# files as positive results, regardless of the contents.
class SainityCheck
	def initialize
		@name = "None"
		@title = nil
		@type = "None"
		@desc = "None"
		@results = Array.new
	end

	attr_reader :name
	attr_reader :type
	attr_reader :desc

	def title
		if @title then
			@title
		else
			@name
		end
	end


	def results
		@results
	end

	# This function will be called for each file, with the argument "file" as the 
	# name of the file and the argument "lines" being an array of Line objects for
	# each line of the file.
	#
	def parse_file(file, lines) 
		raise "Missing parse_file() implementation for Filter: #{@name}"
	end


	private
	
	def add_results( file, lines = [nil] ) 
		lines.each do |line|
			@results << Result.new( self, file, line )
		end
	end
end





class CompareAgainstEmptyString < SainityCheck
	def initialize
		super
		
		@name  = "CmpEmptyString"
		@title = "Comparing With Empty String"
		@type  = "Good Practice"
		@desc  = "Comparisons with empty strings, such as wxT(\"\"), wxEmptyString and "
		@desc += "_(\"\") should be avoided since they force the creation of a temporary "
		@desc += "string object. The proper method is to use the IsEmpty() member-function "
		@desc += "of wxString."
	end

	def parse_file(file, lines) 
		results = lines.select do |line| 
			line.text =~ /[!=]=\s*(wxEmptyString|wxT\(""\)|_\(""\))/ or
			line.text =~ /(wxEmptyString|wxT\(""\)|_\(""\))\s*[!=]=/
		end

		add_results( file, results )
	end
end



class AssignmentToEmptyString < SainityCheck
	def initialize
		super
	
		@name  = "EmptyStringAssignment"
		@title = "Assignment To Empty String"
		@type  = "Good Practice"
		@desc  = "Assigning an empty string such as wxT(\"\"), wxEmptyString and _(\"\") "
		@desc += "to a wxString should be avoided, since it forces the creation of a "
		@desc += "temporary object which is assigned to the string. The proper way to "
		@desc += "clear a string is to use the Clear() member-function of wxString."
	end

	def parse_file(file, lines) 
		if file =~ /\.cpp$/
			results = lines.select do |line| 
				line.text =~ /[^=!]=\s*(wxEmptyString|wxT\(""\)|_\(""\))/ 
			end

			add_results( file, results )
		end
	end
end



class NoIfNDef < SainityCheck
	def initialize
		super
	
		@name  = "NoIfNDef"
		@title = "No #ifndef in headerfile"
		@type  = "Good Practice"
		@desc  = "All header files should contain a #ifndef __<FILENAME>__. The purpuse is to ensure "
		@desc += "that the header can't be included twice, which would introduce a number of problems."
	end

	def parse_file(file, lines)
		if file =~ /\.h$/ then
			if not lines.find { |x| x.text =~ /^#ifndef.*_H/ } then
				add_results( file )
			end
		end
	end
end



class ThisDeference < SainityCheck
	def initialize
		super
	
		@name  = "ThisDeference"
		@title = "Dereference of \"this\""
		@type  = "Good Practice"
		@desc  = "In all but the case of templates, using \"this->\" is unnescesarry and "
		@desc += "only decreases the readability of the code."
	end

	def parse_file(file, lines) 
		results = lines.select do |line|
			line.text =~ /\bthis->/
		end
		
		add_results( file, results )
	end
end



class Assert < SainityCheck
	def initialize
		super
	
		@name  = "Assert"
		@type  = "Consistancy"
		@desc  = "wxASSERT()s should be used rather than normal assert()s "
		@desc += "for the sake of consistancy."
	end

	def parse_file(file, lines) 
		results = lines.select do |line|
			line.text =~ /assert\s*\(/
		end
		
		add_results( file, results )
	end
end



class PassByValue < SainityCheck
	def initialize
		super
	
		@name  = "PassByValue"
		@title = "Pass By Value"
		@type  = "Good Practice"
		@desc  = "Passing objects by value means an extra overhead for large datatypes. "
		@desc += "Therefore should these always be passed by const reference when possible."
		@desc += "Non-const references should only be used for functions where the function is "
		@desc += "supposed to change the actual value of the argument and return another or no value."
	end

	def parse_file(file, lines)
		results = Array.new
	
		# Only handle header files
		if file =~ /\.h$/
			# Items that should be passed by const-ref
			items = [ "wxString", "wxRect", "wxPoint", "CMD4Hash" ]

			lines.each do |line|
				# Try to identify function definitions
				if line.text =~ /^\s*(virtual|static|inline|)\s*\w+\s+\w+\s*\(.*\)/
					# Split by arguments
					line.text.match(/\(.*\)/)[0].split(",").each do |str|
						items.each do |item|
							if str =~ /#{item}\s*[^\s\*]/ and not str =~ /const/
								results.push( line )
							end
						end
					end
				end
			end
		end

		add_results( file, results )
	end
end



class CStr < SainityCheck
	def initialize
		super
	
		@name  = "CStr"
		@title = "C_Str or GetData"
		@type  = "Unicoding"
		@desc  = "Checks for usage of c_str() or GetData(). Using c_str will often result in "
		@desc += "problems on Unicoded builds and should therefore be avoided. "
		@desc += "Please note that the GetData check isn't that precise, because many other "
		@desc += "classes have GetData members, so it does some crude filtering."
	end

	def parse_file(file, lines) 
		results = lines.select do |line| 
			if line.text =~ /c_str\(\)/ 
				true
			else
				line.text =~ /GetData\(\)/ and line.text =~ /(wxT\(|wxString|_\()/
			end
		end

		add_results( file, results )
	end
end



class IfNotDefined < SainityCheck
	def initialize
		super
	
		@name  = "IfDefined"
		@title = "#if (!)defined"
		@type  = "Consistancy"
		@desc  = "Use #ifndef or #ifdef instead for reasons of simplicity."
	end

	def parse_file(file, lines) 
		results = lines.select do |line|
			if line.text =~ /^#if.*[\!]?defined\(/
				not line.text =~ /(\&\&|\|\|)/
			end
		end

		add_results( file, results )
	end
end



class GPLLicense < SainityCheck
	def initialize
		super

		@name  = "MissingGPL"
		@title = "Missing GPL License"
		@type  = "License"
		@desc  = "All header files should contain the proper GPL blorb."
	end

	def parse_file(file, lines)
		if file =~ /\.h$/
			if lines.find { |x| x.text =~ /This program is free software;/ } == nil
				add_results( file )
			end
		end
	end
end



class Copyright < SainityCheck
	def initialize
		super
		
		@name  = "MissingCopyright"
		@title = "Missing Copyright Notice"
		@type  = "License"
		@desc  = "All files should contain the proper Copyright notice."
	end

	def parse_file(file, lines)
		if file =~ /\.h$/
			found = lines.select do |line|
				line.text =~ /Copyright\s*\([cC]\)\s*[-\d,]+ aMule (Project|Team)/
			end
			
			if found.empty? then
				add_results( file )
			end
		end
	end
end



class PartOfAmule < SainityCheck
	def initialize
		super
	
		@name  = "aMuleNotice"
		@title = "Missing aMule notice"
		@type  = "License"
		@desc  = "All files should contain a notice that they are part of the aMule project."
	end

	def parse_file(file, lines)
		if file =~ /\.h$/
			found = lines.select do |line|
				line.text =~ /This file is part of the aMule Project/
			end
			
			if found.empty? then
				add_results( file )
			end
		end
	end
end



class MissingBody < SainityCheck
	def initialize
		super
		
		@name  = "MissingBody"
		@title = "Missing Body in Loop"
		@type  = "Garbage"
		@desc  = "This checks looks for loops without any body. For example \"while(true);\" "
		@desc += "In most cases this is a sign of either useless code or bugs. Only in a few "
		@desc += "cases is it valid code, and in those it can often be represented clearer "
		@desc += "in other ways."
	end

	def parse_file(file, lines) 
		results = lines.select do |line|
			if line.text =~ /^[^}]*while\s*\(.*\)\s*;/ or
			   line.text =~ /^\s*for\s*\(.*\)\s*;[^\)]*$/
				# Avoid returning "for" spanning multiple lines
				# TODO A better way to count instances 
				line.text.split("(").size == line.text.split(")").size
			else
				false
			end
		end

		add_results( file, results )
	end
end



class Translation < SainityCheck
	def initialize
		super
	
		@name  = "Translation"
		@type  = "Consistancy"
		@desc  = "Calls to AddLogLineM should translate the message, whereas "
		@desc += "calls to AddDebugLogLine shouldn't. This is because the user "
		@desc += "is meant to see normal log lines, whereas the the debug-lines "
		@desc += "are only meant for the developers and I dont know about you, but "
		@desc += "I dont plan on learning every language we choose to translate "
		@desc += "aMule to. :P"
	end

	def parse_file(file, lines) 
		results = lines.select do |line|
			if line.text =~ /\"/
				line.text =~ /AddLogLine(M|)\(.*wxT\(/ or
				line.text =~ /AddDebugLogLine(M|)\(.*_\(/
			else
				false
			end
		end

		add_results( file, results )
	end
end



class IfZero < SainityCheck
	def initialize
		super
	
		@name  = "PreIfConstant"
		@title = "#if 0-9"
		@type  = "Garbage"
		@desc  = "Disabled code should be removed as soon as possible. If you wish to disable code "
		@desc += "for only a short period, then please add a comment before the #if. Code with #if [1-9] "
		@desc += "should be left, but the #ifs removed unless there is a pressing need for them."
	end

	def parse_file(file, lines) 
		results = lines.select do |line|
			line.text =~ /#if\s+[0-9]/
		end

		add_results( file, results )
	end
end



class InlinedIfTrue < SainityCheck
	def initialize
		super
	
		@name = "InlinedIf"
		@name = "Inlined If true/false"
		@type = "Garbage"
		@desc = "Using variations of (x ? true : false) or (x ? false : true) is just plain stupid."
	end

	def parse_file(file, lines) 
		results = lines.select do |line|
			line.text =~ /\?\s*(true|false)\s*:\s*(true|false)/i			
		end

		add_results( file, results )
	end
end



class LoopOnConstant < SainityCheck
	def initialize
		super
	
		@name  = "LoopingOnConstant"
		@title = "Looping On Constant"
		@type  = "Garbage"
		@desc  = "This checks detects loops that evaluate constant values "
		@desc += "(true,false,0..) or \"for\" loops with no conditionals. all "
		@desc += "are often a sign of poor code and in most cases can be avoided "
		@desc += "or replaced with more elegant code."
	end

	def parse_file(file, lines) 
		results = lines.select do |line|
			line.text =~ /while\s*\(\s*([0-9]+|true|false)\s*\)/i or
			line.text =~ /for\s*\([^;]*;\s*;[^;]*\)/
		end

		add_results( file, results )
	end
end



class MissingImplementation < SainityCheck
	def initialize
		super

		@name  = "MissingImplementation"
		@title = "Missing Function Implementation"
		@type  = "Garbage"
		@desc  = "Forgetting to remove a function-definition after having removed it "
		@desc += "from the .cpp file only leads to cluttered header files. The only "
		@desc += "case where non-implemented functions should be used is when it is "
		@desc += "necesarry to prevent usage of for instance assignment between "
		@desc += "instances of a class."

		@declarations = Array.new
		@definitions = Array.new
	end

	def results
		@definitions.each do |definition|
			@declarations.delete_if do |pair|
				pair.first == definition
			end
		end
		
		
		return @declarations.map do |pair|
			Result.new( self, pair.last.first, pair.last.last )
		end
	end

	def parse_file(file, lines)
		if file =~ /\.h$/ then
			level = 0
			tree = Array.new
			
			lines.each do |line|
				level += line.text.count( "{" ) - line.text.count( "}" )
			
				tree.delete_if do |struct| 
					struct.first > level
				end

				
				if line.text !~ /^\s*\/\// and line.text !~ /^\s*#/ and line.text !~ /typedef/ then
					if line.text =~ /^\s*(class|struct)\s+/ and line.text.count( ";" ) == 0 then
						cur_level = level;
						
						if line.text.count( "{" ) == 0 then
							cur_level += 1
						end
						
						entry = [ cur_level, line.text.scan( /^\s*(class|struct)\s+([^\:{;]+)/ ).first.last.split.last ]
				
						tree << entry
					elsif line.text =~ /;\s*$/ and line.text.count( "{" ) == 0 then
						# No pure virtual functions and no return(blah) calls (which otherwise can fit the requirements)
						if line.text !~ /=\s*0\s*;\s*$/ and line.text !~ /return/ then
							re = /^\s*(virtual\s+|static\s+|inline\s+|)\w+(\s+[\*\&]?|[\*\&]\s+)(\w+)\(/.match( line.text )
	
							if re and level > 0 and tree.last then
								@declarations << [ tree.last.last + "::" + re[ re.length - 1 ], [ file, line ] ]
							end
						end
					end
				end
			end
		else
			lines.each do |line|
				if line.text =~ /\b\w+::\w+\s*\([^;]+$/
					@definitions << line.text.scan( /\b(\w+::\w+)\s*\([^;]+$/ ).first.first
				end
			end	
		end
	end
end







# List of enabled filters
filterList = Array.new
filterList.push CompareAgainstEmptyString.new
filterList.push AssignmentToEmptyString.new
filterList.push NoIfNDef.new
filterList.push ThisDeference.new
filterList.push Assert.new
filterList.push PassByValue.new
filterList.push CStr.new
filterList.push IfNotDefined.new
filterList.push GPLLicense.new
filterList.push Copyright.new
filterList.push PartOfAmule.new
filterList.push MissingBody.new
filterList.push Translation.new
filterList.push IfZero.new
filterList.push InlinedIfTrue.new
filterList.push LoopOnConstant.new
filterList.push MissingImplementation.new


# Sort enabled filters by type and name. The reason why this is done here is
# because it's much easier than manually resorting every time I add a filter
# or change the name or type of an existing filter.
filterList.sort! do |x,y|
	cmp = x.type <=> y.type
	
	if cmp == 0 then
		x.title <=> y.title
	else
		cmp
	end
end



def parse_files( path, filters )
	filters = filters.dup

	require "find"

	Find.find( path ) do |filename|
		if filename =~ /\.(cpp|h)$/ and not filename =~ /CryptoPP\./
			File.open(filename, "r") do |aFile|
				# Read lines and add line-numbers
				lines = Array.new
				aFile.each_line do |line|
					lines.push( Line.new( aFile.lineno, line ) )
				end

				lines.freeze

				# Check the file against each filter
				filters.each do |filter|
					# Process the file with this filter
					filter.parse_file( filename, lines )
				end
			end
		end
	end

	results = Array.new
	filters.each do |filter|
		results += filter.results
	end

	results
end




# Helper-function
def get_val( key, list )
	if not list.last or list.last.first != key then
		list << [ key, Array.new ]
	end
	
	list.last.last
end



def create_result_tree( path, filters )
	# Gather the results
	results = parse_files( path, filters )
	
	# Sort the results by the following sequence of variables: Path -> File -> Filter -> Line
	results.sort! do |a, b|
		if (a.file_path <=> b.file_path) == 0 then
			if (a.file_name <=> b.file_name) == 0 then
				if (a.type.title <=> b.type.title) == 0 then
					a.line.number <=> b.line.number
				else
					a.type.title <=> b.type.title
				end
			else
				a.file_name <=> b.file_name
			end
		else
			a.file_path <=> b.file_path
		end
	end


	# Create a tree of results: [ Path, [ File, [ Filter, [ Line ] ] ] ]
	result_tree = Array.new
	results.each do |result|
		get_val( result.type, get_val( result.file_name, get_val( result.file_path, result_tree ) ) ) << result
	end


	result_tree
end



def create_filter_tree( filters )
	# Change the filterList to a tree: [ Type, [ Filter ] ]
	filter_tree = Array.new

	filters.each do |filter|
		get_val( filter.type, filter_tree ) << filter
	end

	filter_tree
end



# Converts a number to a string and pads with zeros so that length becomes at least 5
def PadNum( number )
	num = number.to_s

	if ( num.size < 5 ) 
		( "0" * ( 5 - num.size ) ) + num
	else
		num
	end
end



# Helper-function that escapes some chars to HTML codes
def HTMLEsc( str ) 
	str.gsub!( /\&/, "&amp;"  )
	str.gsub!( /\"/, "&quot;" )
	str.gsub!( /</,  "&lt;"   )
	str.gsub!( />/,  "&gt;"   )
	str.gsub!( /\n/, "<br>"   )
	str.gsub( /\t/,  "&nbsp;" )
end



# Fugly output code goes here
# ... Abandon hope, yee who read past here
# TODO Enable use of templates.
# TODO Abandon hope.
def OutputHTML( filters, results )
	text  = 
"<html>
	<head>
		<STYLE TYPE=\"text/css\">
		<!--
			.dir {
				background-color: \#A0A0A0;
				padding-left: 10pt;
				padding-right: 10pt;
			}
			.file {
				background-color: \#838383;
				padding-left: 10pt;
				padding-right: 10pt;
			}
			.filter {
				background-color: #757575;
				padding-left: 10pt;
				padding-right: 10pt;
				padding-top: 5pt;
			}	
		-->
		</STYLE>
	</head>
	<body bgcolor=\"\#BDBDBD\">

		<h1>Filters</h1>
		<dl>
"
	# List the filters
	filters.each do |filterType|
		text += 
"			<dt><b>#{filterType.first}</b></dt>
			<dd>
				<dl>
"
		filterType.last.each do |filter|
			text += 
"					<dt id=\"#{filter.name}\"><i>#{filter.title}</i></dt>
					<dd>
						#{HTMLEsc(filter.desc)}<p>
					</dd>
"
		end

		text +=
"				</dl>
			</dd>
"
	end

	text += 
"		</dl>
	
		<p>
	
		<h1>Directories</h1>
		<ul>
"

	# List the directories
	results.each do |dir|
		text +=
"			<li>
				<a href=\"\##{dir.first}\">#{dir.first}</a>
			</li>
"
	end

	text += 
"		</ul>

		<p>
	
		<h1>Results</h1>
"

	results.each do |dir|
		text += 
"		<div class=\"dir\">
			<h2 id=\"#{dir.first}\">#{dir.first}</h2>
"

		dir.last.each do |file|
			text += 
"			<div class=\"file\">
				<h3>#{file.first}</h3>

				<ul>
"
			
			file.last.each do |filter|
				text += 
"					<li>
						<div class=\"filter\">
							<b><a href=\"\##{filter.first.name}\">#{filter.first.title}</a></b>

							<ul>
"
			
				filter.last.each do |result|
					if result.line then
						text += 
"								<li><b>#{PadNum(result.line.number)}:</b> #{HTMLEsc(result.line.text.strip)}</li>
"
					end
				end

				text +=
"							</ul>
						</div>
					</li>
"
			end
			text += 
"				</ul>
			</div>
"
		end
		
		text += 
"		</div>

		<p>
"
	end

	text += 
"	</body>
</html>"

	return text;
end



# Columnizing, using the http://www.rubygarden.org/ruby?UsingTestUnit example because I'm lazy
# TODO Rewrite it to better support newlines and stuff
def Columnize( text, width, indent )
	return indent + text.scan(/(.{1,#{width}})(?: |$)/).join("\n#{indent}")
end



# Fugly output code also goes here, this is a bit more sparse than the HTML stuff
def OutputTEXT( filters, results )
	
	# List the filters
	text = "Filters\n"
	filters.each do |filterType|
		text += "\t* #{filterType.first}\n"
		
		filterType.last.each do |filter|
			text += "\t\t- #{filter.title}\n"
		
			text += Columnize( filter.desc, 80, "\t\t\t" ) + "\n\n"
		end
	end

	# List the directories
	text += "\n\nDirectories\n"
	results.each do |dir|
		text += "\t#{dir.first}\n"
	end

	text += "\n\nResults\n"

	# To avoid bad readability, I only use fullpaths here instead of sections per dir
	results.each do |dir|
		dir.last.each do |file|
			text += "\t#{dir.first}#{file.first}\n"

			file.last.each do |filter|
				text += "\t\t* #{filter.first.title}\n"

				filter.last.each do |result|
					if result.line then
						text += "\t\t\t#{PadNum(result.line.number)}: #{result.line.text.strip}\n"
					end
				end
			end

			text += "\n"
		end
	end

	return text;
end



#TODO Improved parameter-handling, add =<file> for the outputing to a file
ARGV.each do |param|
	case param
		when "--text" then 
			puts OutputTEXT( create_filter_tree( filterList ), create_result_tree( ".", filterList ) )
		when "--html" then 
			puts OutputHTML( create_filter_tree( filterList ), create_result_tree( ".", filterList ) )
	end
end

