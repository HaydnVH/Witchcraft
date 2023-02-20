--sandbox.lua

--- Sets up tables and functions to allow scripts run by the Witchcraft engine
--- to run in their own protected sandbox environments.
-- Written by Haydn V. Harach on 2016-03-21
-- Last modified on 2023-02-14

-- The 'readonly' function makes it simple to define read-only tables.
function readonly(tbl)
	return setmetatable({}, {
	__index = tbl,
	__newindex = function(tbl, key, val)
		error("Attempting to write to read-only table.", 3) return end,
	__metatable = false
	})
end

-- The 'showtable' function allows us to print the entire table's contents - including the contents of any tables inside!
showtable = function(t, depth, set)

	if not depth then
		depth = ""
	end
	
	if not set then
		set = {}
	end
	
	--set[t] = true
	
	for k,v in pairs(t) do
		if type(v) == 'table' then
			print(depth, k, ":")
			if set[t] then
				print(depth, k, ": (already encountered)")
			else
				showtable(v, depth.."  ", set)
			end
		else
			print(depth, k, ": ", v)
		end
	end
	
end

-- The 'SANDBOX' table contains everything from _G that we want user-run scripts to have access to.
-- It also contains proxy tables that give user scripts read-only access to other user scripts.
SANDBOX = {

	assert = _G.assert,
	dofile = _G.dofile, -- Engine Redefined
	error = _G.error,
	ipairs = _G.ipairs,
	next = _G.next,
	pairs = _G.pairs,
	pcall = _G.pcall,
	print = _G.print, -- Engine Redefined
	printmore = _G.printmore,
--	require = _G.require, -- Redefined
	select = _G.select,
	tonumber = _G.tonumber,
	tostring = _G.tostring,
	type = _G.type,
	unpack = _G.unpack,
	_VERSION = _G._VERSION,
	xpcall = _G.xpcall,
	
	readonly = _G.readonly,
	showtable = _G.showtable,
	
	-- For tables inside _G that we want access to, we create proxy tables which provide
	-- read-only access to only the functions and variables that we want.
	coroutine = readonly({
		create = _G.coroutine.create,
		resume = _G.coroutine.resume,
		running = _G.coroutine.running,
		status = _G.coroutine.status,
		wrap = _G.coroutine.wrap,
		yield = _G.coroutine.yield
	}),
	
	math = readonly({
		abs = _G.math.abs,
		acos = _G.math.acos,
		asin = _G.math.asin,
		atan = _G.math.atan,
		atan2 = _G.math.atan2,
		ceil = _G.math.ceil,
		cos = _G.math.cos,
		cosh = _G.math.cosh,
		deg = _G.math.deg,
		exp = _G.math.exp,
		floor = _G.math.floor,
		fmod = _G.math.fmod,
		frexp = _G.math.frexp,
		huge = _G.math.huge,
		ldexp = _G.math.ldexp,
		log = _G.math.log,
		log10 = _G.math.log10,
		max = _G.math.max,
		min = _G.math.min,
		modf = _G.math.modf,
		pi = _G.math.pi,
		pow = _G.math.pow,
		rad = _G.math.rad,
		random = _G.math.random, -- TODO: Create a better, instanceable RNG?
		sin = _G.math.sin,
		sinh = _G.math.sinh,
		sqrt = _G.math.sqrt,
		tan = _G.math.tan,
		tanh = _G.math.tanh
	}),

	math2 = readonly({
		tau = _G.math2.tau,
		deg2rad = _G.math2.deg2rad,
		rad2deg = _G.math2.rad2deg,

		angle_diff = _G.math2.angle_difference,
		radians_diff = _G.math2.radians_difference,
		clamp = _G.math2.clamp,
		lerp = _G.math2.lerp,
		linstep = _G.math2.linstep,
		sign = _G.math2.sign,
		spring = _G.math2.spring,
		spring_angle = _G.math2.spring_angle,
		spring_radians = _G.math2.spring_radians
	}),
	
	os = readonly({
		clock = _G.os.clock,
		date = _G.os.date,
		difftime = _G.os.difftime,
		time = _G.os.time
	}),
	
	string = readonly({
		byte = _G.string.byte,
		char = _G.string.char,
		find = _G.string.find,
		format = _G.string.format,
		gmatch = _G.string.gmatch,
		gsub = _G.string.gsub,
		len = _G.string.len,
		lower = _G.string.lower,
		match = _G.string.match,
		rep = _G.string.rep,
		reverse = _G.string.reverse,
		sub = _G.string.sub,
		upper = _G.string.upper
	}),
	
	table = readonly({
		insert = _G.table.insert,
		maxn = _G.table.maxn,
		remove = _G.table.remove,
		sort = _G.table.sort,
		unpack = _G.table.unpack,
	}),
}

-- The 'SCRIPT_ENV' table holds the tables which will be the environment for user-run scripts.
-- Each script gets it's own environment, shared only by scripts with the same name.
-- To clear or re-initialize the script environment (for testing) set SCRIPT_ENV[scriptname] to an empty table.
SCRIPT_ENV = {}

-- Every script environment has the same metatable, so we save it in _G to re-use.
-- While the script runs in it's own environment, 'SCRIPT_ENV.scriptName',
-- it has access to 'SANDBOX' through the __index metamethod.
-- __newindex prevents the script from creating a value with the same key as something in the sandbox,
-- as doing so would make that feature inaccessible.
SCRIPT_ENV_MT = {
	__index = SANDBOX,
	__newindex = function(tbl, key, val)
		if SANDBOX[key] ~= nil then
			error("Attempting to overwrite protected key '"..tostring(key).."'.", 3)
			return end
		rawset(tbl, key, val)
	end,
	__metatable = false
}

-- The 'CONSOLE' table is the environment used by lua strings run in the console prompt.
-- 'CONSOLE_PROTECTED' will contain any functions usable from the console that aren't usable by normal scripts.
CONSOLE_PROTECTED = setmetatable({},
{
	__index = SANDBOX,
	__metatable = false
})

CONSOLE_MT = {
	__index = CONSOLE_PROTECTED,
	__newindex = function(tbl, key, val)
	if CONSOLE_PROTECTED[key] ~= nil then
		error("Attempting to overwrite protected key '"..tostring(key).."'.", 3)
		return
		end
	rawset(tbl, key, val)
	end,
	__metatable = false
}

CONSOLE = setmetatable({}, CONSOLE_MT)

CONSOLE_PROTECTED.reset_console = function()
	CONSOLE = setmetatable({}, CONSOLE_MT)
end

-- setup_script_env() is run by the engine before a user script is run, setting up the environment for it.
function setup_script_env(scriptname)

	-- If we've set up a table for 'scriptname' before, we bail out now.
	if SCRIPT_ENV[scriptname] then return end
	
	-- Create the table for 'scriptname', and a protected table as well.
	SCRIPT_ENV[scriptname] = setmetatable({}, SCRIPT_ENV_MT)
end