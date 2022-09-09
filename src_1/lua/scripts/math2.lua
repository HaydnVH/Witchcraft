-- math2.lua

math2 = {

	tau = math.pi * 2,
	deg2rad = math.pi / 180,
	rad2deg = 180 / math.pi,

	spring = function(current, velocity, target, tightness, delta_time)
		local current_to_target = target - current
		local spring_force = current_to_target * tightness
		local damping_force = -velocity * 2 * math.sqrt(tightness)
		local force = spring_force + damping_force
		velocity = velocity + (force * delta_time)
		local displacement = velocity * delta_time
		current = current + displacement
		return current, velocity
	end,

	angle_difference = function(lhs, rhs)
		local diff = (((rhs - lhs) + 180) % 360) - 180
		if diff < -180 then
			return diff + 360
		else
			return diff
		end
	end,

	radians_difference = function(lhs, rhs)
		local diff = (((rhs - lhs) + math.pi) % (math.pi * 2) - math.pi)
		if diff < -math.pi then
			return diff + (math.pi * 2)
		else
			return diff
		end
	end,

	spring_angle = function(current, velocity, target, tightness, delta_time)
		local current_to_target = angle_difference(current, target)
		local spring_force = current_to_target * tightness
		local damping_force = -velocity * 2 * math.sqrt(tightness)
		local force = spring_force + damping_force
		velocity = velocity + (force * delta_time)
		local displacement = velocity * delta_time
		current = current + displacement
		return current, velocity
	end,

	spring_radians = function(current, velocity, target, tightness, delta_time)
		local current_to_target = radians_difference(current, target)
		local spring_force = current_to_target * tightness
		local damping_force = -velocity * 2 * math.sqrt(tightness)
		local force = spring_force + damping_force
		velocity = velocity + (force * delta_time)
		local displacement = velocity * delta_time
		current = current + displacement
		return current, velocity
	end,

	sign = function(val)
		if val < 0 then return -1 else return 1 end
	end,

	clamp = function(val, minimum, maximum)
		if maximum < minimum then minimum, maximum = maximum, minimum end
		return math.min(maximum, math.max(minimum, val))
	end,

	linstep = function(value, minimum, maximum)
		return clamp((value-minimum) / (maximum - minimum), 0, 1)
	end,

	lerp = function(from, to, alpha)
		return (from * (1 - alpha)) + (to * alpha)
	end

}