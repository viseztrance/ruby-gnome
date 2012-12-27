# This sample code is a port of clutter/examples/basic-actor.c.
# It is licensed under the terms of the GNU Lesser General Public
# License, version 2.1 or (at your option) later.
#
# Copyright (C) 2012  Ruby-GNOME2 Project Team
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

require "clutter"

Clutter.init(ARGV.size, ARGV)

stage = Clutter::Stage.new
stage.signal_connect("destroy") do |*args|
  Clutter.main_quit
end

vase = Clutter::Actor.new
vase.name = "vase"
vase.layout_manager = Clutter::BoxLayout.new
vase.background_color = Clutter::Color.get_static(:sky_blue_light)
vase.add_constraint(Clutter::AlignConstraint.new(stage, :both, 0.5))
stage.add_child(vase)

SIZE = 128

flowers = []

flower = Clutter::Actor.new
flowers << flower
flower.name = "flower.1"
flower.set_size(SIZE, SIZE)
flower.margin_left = 12
flower.background_color = Clutter::Color.get_static(:red)
flower.reactive = true
vase.add_child(flower)

toggled = true
flower.signal_connect("button-press-event") do |actor, event|
  if toggled
    end_color = Clutter::Color.get_static(:blue)
  else
    end_color = Clutter::Color.get_static(:red)
  end

  actor.save_easing_state
  actor.set_easing_duration(500)
  actor.set_easing_mode(:linear)
  actor.background_color = end_color
  actor.restore_easing_state

  toggled = !toggled

  stop = true
  stop
end

flower = Clutter::Actor.new
flowers << flower
flower.set_name("flower.2")
flower.set_size(SIZE, SIZE)
flower.set_margin_top(12)
flower.set_margin_left(6)
flower.set_margin_right(6)
flower.set_margin_bottom(12)
flower.set_background_color(Clutter::Color.get_static(:yellow))
flower.set_reactive(flower, true)
vase.add_child(flower)

on_crossing = lambda do |actor, event|
  if event.type == Clutter::EventType::ENTER
    zpos = -250.0
  else
    zpos = 0.0
  end

  actor.save_easing_state
  actor.set_easing_duration(500)
  actor.set_easing_mode(:ease_out_bounce)
  actor.set_z_position(zpos)
  actor.restore_easing_state

  stop = true
  stop
end
flower.signal_connect("enter-event", &on_crossing)
flower.signal_connect("leave-event", &on_crossing)

flower = Clutter::Actor.new
flowers << flower
flower.set_name("flower.3")
flower.set_size(SIZE, SIZE)
flower.set_margin_right(12)
flower.set_background_color(Clutter::Color::get_static(:green))
flower.set_pivot_point(0.5, 0.0)
flower.set_reactive(true)
vase.add_child(flower);
flower.signal_connect("button-press-event") do |actor, event|
  actor.save_easing_state
  actor.set_easing_duration(1000)

  actor.set_rotation_angle(:y_axis, 360.0)

  actor.restore_easing_state

  id = actor.signal_connect("transition-stopped::rotation-angle-y") do
    actor.save_easing_state
    actor.set_rotation_angle(:y_axis, 0.0)
    actor.restore_easing_state

    actor.signal_handler_disconnect(id)
  end

  stop = true
  stop
end

stage.show

Clutter.main
