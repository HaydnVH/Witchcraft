--events.lua

EVENTS = {}
SANDBOX.events = readonly(EVENTS)

local listeners = {}

function GENERIC_EVENT_LISTEN(self, id, scriptname, func)
	if listeners[self.eventid][id] == nil then
		listeners[self.eventid][id] = {}
	end
	listeners[self.eventid][id][scriptname] = func
end

function GENERIC_EVENT_UNLISTEN(self, id, scriptname)
	if listeners[self.eventid][id] == nil then return end
	listeners[self.eventid][id][scriptname] = nil
end

function GENERIC_EVENT_EXECUTE_GLOBAL(self, ...)
	for id, T in pairs(listeners[self.eventid]) do
		for scriptname, func in pairs(T) do
			local success, error
			if func ~= nil then
				success, error = pcall(func, id, ...)
			else
				print("Error executing '"..scriptname.."'.'"..self.eventid.."'("..tostring(id).."):\n Callback function is nil.")
				listeners[self.eventid][id][scriptname] = nil
			end
			if success == false then
				print("Error executing '"..scriptname.."'.'"..self.eventid.."'("..tostring(id).."):\n", error)
				listeners[self.eventid][id][scriptname] = nil
			end
		end
	end
end

function GENERIC_EVENT_EXECUTE_LOCAL(self, id, ...)
	if listeners[self.eventid][id] == nil then return end
	for scriptname, func in pairs(listeners[self.eventid][id]) do
		local success, error
		if func ~= nil then
			success, error = pcall(func, id, ...)
		else
			print("Error executing '"..scriptname.."'.'"..self.eventid.."'("..tostring(id).."):\n Callback function is nil.")
			listeners[self.eventid][id][scriptname] = nil
		end
		if success == false then
			print("Error executing '"..scriptname.."'.'"..self.eventid.."'("..tostring(id).."):\n", error)
			listeners[self.eventid][id][scriptname] = nil
		end
	end
end

function CREATE_GENERIC_EVENT(eventname)
	listeners[eventname] = {}
	return readonly({
		eventid = eventname,
		listen = GENERIC_EVENT_LISTEN,
		unlisten = GENERIC_EVENT_UNLISTEN,
		execute_global = GENERIC_EVENT_EXECUTE_GLOBAL,
		execute_local = GENERIC_EVENT_EXECUTE_LOCAL,
	})
end

EVENTS.create_generic_event = CREATE_GENERIC_EVENT
EVENTS.logical_update = CREATE_GENERIC_EVENT("logical_update")
EVENTS.display_update = CREATE_GENERIC_EVENT("display_update")
EVENTS.animation_event = CREATE_GENERIC_EVENT("animation_event")
EVENTS.entity_deletion = CREATE_GENERIC_EVENT("entity_deletion")