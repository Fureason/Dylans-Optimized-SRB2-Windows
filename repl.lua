if type(loadstring) ~= 'function' then
	error('loadstring is not a function. Are you running the correct SRB2 build?')
end

local function does_global_exist(key)
	if #key == 1 and 'a' <= key and key <= 'z' then
		return false --technically they do, but we'll pretend they don't for convenience sake
	else
		return _G[key] ~= nil
	end
end

local env
local env_meta = {
	_G=_G,       -- enable access to _G
	__index=_G,  -- enable reading global variables without _G
	__newindex=function(t,k,v)
		-- pass assignments to nonexistent values onto their global variables if present; otherwise save them in env
		if does_global_exist(k) then
			_G[k] = v
		else
			rawset(env, k, v)
		end
	end
}

local function print_error(text)
	print('\x85'..'ERROR: \x80'..text)
end

local interval_meta = {
	__tostring=function(int)
		local str
		if int.enabled then
			str = '<I='
		else
			str = '<i='
		end
		return str..tostring(int.time)..': '..tostring(int.str)..'>'
	end,
	__call=function(int, ...)
		local s,r = pcall(int.func, ...)
		if s then
			return r
		else
			int.enabled = false
			print_error(r)
		end
	end
}

local watch_meta = {
	__tostring=function(watch)
		local str
		if watch.enabled then
			str = '<W: '
		else
			str = '<w: '
		end
		if watch.label then
			str = $..'"'..watch.label..'" '
		end
		return str..watch.str..'>'
	end,
	__call=function(watch, ...)
		local s,r = pcall(watch.func, ...)
		if s then
			return r
		else
			if not watch.suppress_errors then
				print_error(r)
				watch.suppress_errors = true
			end
			return '\x85'..'ERROR'
		end
	end
}

local list_meta = {
	__tostring=function(list)
		if #list == 0 then
			return '(empty list)'
		else
			local str = '\n'
			for i,v in ipairs(list) do
				str = $..tostring(i)..'\t'..tostring(v)..'\n'
			end
			return str
		end
	end
}

local function process_func_arg(arg, arg_desc, prepend)
	if type(arg) == 'string' then
		prepend = $ or ''
		local f,e = loadstring(prepend..arg)
		if f then
			setfenv(f, env)
			return f
		else
			error(e)
		end
	elseif type(arg) == 'function' then
		return arg
	else
		error('Must specify a string or a function as '..arg_desc)
	end
end

local function reset_env()
	env = {out={}, intervals={}, watches={}}
	env.hud_x = 0
	env.hud_y = 132
	env.hud_flags = V_HUDTRANS|V_SMALLSCALEPATCH|V_ALLOWLOWERCASE
	setmetatable(env.out, list_meta)
	setmetatable(env.intervals, list_meta)
	setmetatable(env.watches, list_meta)

	env.interval = function(time, func)
		local str = tostring(func)
		if type(time) ~= 'number' then
			error('Must specify a number as the first argument to interval')
		end
		func = process_func_arg(func, 'the second argument to interval')

		local int = {time=time, func=func, enabled=true, timeleft=time, str=str}

		int.on = function() int.enabled = true end
		int.off = function() int.enabled = false end
		
		int.append = function(func2)
			local str2 = tostring(func2)
			func2 = process_func_arg(func2, 'the argument to append')
			int.str = $..'; '..str2
			local oldfunc = int.func
			int.func = function()
				oldfunc()
				func2()
			end
		end

		setmetatable(int, interval_meta)
		table.insert(env.intervals, int)
		return int
	end

	env.watch = function(func, label)
		local str = tostring(func)
		if label ~= nil then
			label = tostring($)
		end
		func = process_func_arg(func, 'the first argument to watch', 'return ')
		local watch = {func=func, str=str, label=label, enabled=true}
		watch.on = function()
			watch.enabled = true
			watch.suppress_errors = false
		end
		watch.off = function() watch.enabled = false end
		setmetatable(watch, watch_meta)
		table.insert(env.watches, watch)
		return watch
	end

	env.spawn = function(mt, dist, z)
		if dist == nil then
			dist = 128
		elseif dist >= FRACUNIT then
			dist = $/FRACUNIT
		end

		if z == nil then
			z = 0
		elseif z < FRACUNIT then
			z = $*FRACUNIT
		end

		local x = dist * cos(consoleplayer.mo.angle)
		local y = dist * sin(consoleplayer.mo.angle)
		local mo = P_SpawnMobjFromMobj(consoleplayer.mo, x, y, z, mt)
		mo.angle = ANGLE_180 + consoleplayer.mo.angle
		return mo
	end

	env.fu = function(value)
		local str = tostring(value/FRACUNIT)..'FU'
		local frac = value % FRACUNIT
		local fstr = tostring(frac)
		if frac > 0 then
			return str..'+'..fstr
		elseif frac < 0 then
			return str..fstr
		else
			return str
		end
	end

	setmetatable(env, env_meta)
end
reset_env()

COM_AddCommand('lua', function(player, ...)
	local arg = ''
	for i,v in ipairs({...}) do
		if i > 1 then
			arg = $..' '
		end
		arg = $..v
	end

	local f,e = loadstring('return '..arg)
	if not f then
		f,e = loadstring(arg)
		if not f then
			print_error(e)
			return
		end
	end

	env.p = player
	env.m = nil
	env.s = nil
	if player and player.valid and player.mo then
		env.m = player.mo
		if player.mo.valid then
			env.s = player.mo.subsector.sector
		end
	end
	
	if type(f) == 'string' then print(f) end
	setfenv(f, env)
	local s,r = pcall(f)

	if s then
		if r ~= nil then
			table.insert(env.out, r)
			rawset(env, '_', r)
			local udtype = type(r)
			if udtype == 'userdata' then
				udtype = userdataType(r)
			end
			print('\x82out['..tostring(#env.out)..'] = \x80'..tostring(r)..'\x83 ('..udtype..')')
		end
	else
		print_error(r)
	end
end, COM_LOCAL)

COM_AddCommand('lua_reset', reset_env, COM_LOCAL)

addHook('ThinkFrame', function()
	for i,v in ipairs(env.intervals) do
		if v.enabled then
			if v.timeleft == 1 then
				v()
				v.timeleft = v.time
			else
				v.timeleft = $-1
			end
		end
	end
end)

local function watch_hud(v)
	local ROW_HEIGHT = 10
	local y = env.hud_y
	for i,w in ipairs(env.watches) do
		if w.enabled then
			local label = w.label or (w.str..'=')
			v.drawString(env.hud_x, y, '\x82'..label, env.hud_flags)
			v.drawString(env.hud_x, y+ROW_HEIGHT, tostring(w()), env.hud_flags)
			y = $ + 2*ROW_HEIGHT
		end
	end
end

for i,v in ipairs({'game', 'scores', 'title', 'titlecard', 'intermission'}) do
	hud.add(watch_hud, v)
end
