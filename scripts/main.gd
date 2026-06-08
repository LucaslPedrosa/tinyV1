extends Node2D

const MAP_RECT := Rect2(Vector2.ZERO, Vector2(3200.0, 2200.0))
const CAMERA_START := Vector2(780.0, 1100.0)
const CAMERA_ZOOM_LEVELS: Array[float] = [-1.0, -0.5, 0.1, 0.5, 0.667, 1.0, 1.5, 2.0]
const CAMERA_WHEEL_ZOOM_STEP := 1
const CAMERA_START_ZOOM_INDEX := 4
const CAMERA_EDGE_MARGIN := 40.0
const CAMERA_EDGE_SPEED := 620.0

var game: Node
var game_camera: Camera2D
var camera_zoom_index := CAMERA_START_ZOOM_INDEX
var portrait_cache: Dictionary[String, Texture2D] = {}
var icon_cache: Dictionary[String, Texture2D] = {}

@onready var resources_label: Label = $HudLayer/BottomHud/ResourcesBox/ResourcesLabel
@onready var bottom_hud: Control = $HudLayer/BottomHud
@onready var actions_label: Label = $HudLayer/BottomHud/ActionsBox/ActionsLabel
@onready var action_button_0: Button = $HudLayer/BottomHud/ActionsBox/ActionButton0
@onready var action_button_1: Button = $HudLayer/BottomHud/ActionsBox/ActionButton1
@onready var portrait: TextureRect = $HudLayer/BottomHud/SelectionBox/Portrait
@onready var name_label: Label = $HudLayer/BottomHud/SelectionBox/NameLabel
@onready var details_label: Label = $HudLayer/BottomHud/SelectionBox/DetailsLabel


func _ready() -> void:
	texture_filter = CanvasItem.TEXTURE_FILTER_NEAREST
	bottom_hud.texture_filter = CanvasItem.TEXTURE_FILTER_NEAREST
	portrait.texture_filter = CanvasItem.TEXTURE_FILTER_NEAREST
	action_button_0.texture_filter = CanvasItem.TEXTURE_FILTER_NEAREST
	action_button_1.texture_filter = CanvasItem.TEXTURE_FILTER_NEAREST
	action_button_0.pressed.connect(_on_action_button_pressed.bind(0))
	action_button_1.pressed.connect(_on_action_button_pressed.bind(1))
	_setup_camera()

	if not ClassDB.class_exists(&"TinyGame"):
		GDExtensionManager.load_extension("res://tinyv1.gdextension")

	if ClassDB.class_exists(&"TinyGame"):
		game = ClassDB.instantiate(&"TinyGame") as Node
		add_child(game)
		move_child(game, 0)
		(game as CanvasItem).z_index = -10
		_update_hud()
	else:
		resources_label.text = "Build the C++ extension next"


func _process(_delta: float) -> void:
	_update_camera(_delta)
	if game != null:
		_update_hud()


func _setup_camera() -> void:
	game_camera = Camera2D.new()
	game_camera.name = "GameCamera"
	game_camera.position = CAMERA_START.round()
	_set_camera_zoom_by_index(CAMERA_START_ZOOM_INDEX)
	game_camera.position_smoothing_enabled = false
	add_child(game_camera)
	game_camera.make_current()
	game_camera.position = _clamp_camera_position(game_camera.position, get_viewport_rect().size)


func _input(event: InputEvent) -> void:
	if game_camera == null:
		return

	var mouse_event := event as InputEventMouseButton
	if mouse_event == null or not mouse_event.pressed:
		return

	if mouse_event.button_index == MOUSE_BUTTON_WHEEL_UP:
		_change_camera_zoom(CAMERA_WHEEL_ZOOM_STEP)
		get_viewport().set_input_as_handled()
	elif mouse_event.button_index == MOUSE_BUTTON_WHEEL_DOWN:
		_change_camera_zoom(-CAMERA_WHEEL_ZOOM_STEP)
		get_viewport().set_input_as_handled()

func _change_camera_zoom(direction: int) -> void:
	_set_camera_zoom_by_index(camera_zoom_index + direction)

func _set_camera_zoom_by_index(index: int) -> void:
	camera_zoom_index = clampi(index, 0, CAMERA_ZOOM_LEVELS.size() - 1)
	var zoom := CAMERA_ZOOM_LEVELS[camera_zoom_index]
	game_camera.zoom = Vector2(zoom, zoom)
	game_camera.position = _clamp_camera_position(game_camera.position, get_viewport_rect().size)

func _update_camera(delta: float) -> void:
	if game_camera == null:
		return

	var viewport_size := get_viewport_rect().size
	var mouse_position := get_viewport().get_mouse_position()
	var playable_bottom := viewport_size.y - bottom_hud.size.y
	var direction := Vector2.ZERO

	if mouse_position.x <= CAMERA_EDGE_MARGIN:
		direction.x -= 1.0
	elif mouse_position.x >= viewport_size.x - CAMERA_EDGE_MARGIN:
		direction.x += 1.0

	if mouse_position.y <= CAMERA_EDGE_MARGIN:
		direction.y -= 1.0
	elif mouse_position.y >= playable_bottom - CAMERA_EDGE_MARGIN and mouse_position.y <= playable_bottom:
		direction.y += 1.0

	if direction == Vector2.ZERO:
		return

	game_camera.position += direction.normalized() * CAMERA_EDGE_SPEED * delta
	game_camera.position = _clamp_camera_position(game_camera.position, viewport_size)


func _clamp_camera_position(position: Vector2, viewport_size: Vector2) -> Vector2:
	var playable_viewport_size := Vector2(viewport_size.x, viewport_size.y - bottom_hud.size.y)
	var visible_size := playable_viewport_size / game_camera.zoom
	var half_visible := visible_size * 0.5
	var min_position := MAP_RECT.position + half_visible
	var max_position := MAP_RECT.position + MAP_RECT.size - half_visible

	if min_position.x > max_position.x:
		position.x = MAP_RECT.get_center().x
	else:
		position.x = clampf(position.x, min_position.x, max_position.x)

	if min_position.y > max_position.y:
		position.y = MAP_RECT.get_center().y
	else:
		position.y = clampf(position.y, min_position.y, max_position.y)

	return _snap_camera_position(position)

func _snap_camera_position(position: Vector2) -> Vector2:
	var zoom := absf(game_camera.zoom.x)
	if zoom <= 0.0:
		return position.round()

	var snap_step := 1.0 / zoom
	return Vector2(snappedf(position.x, snap_step), snappedf(position.y, snap_step))


func _update_hud() -> void:
	resources_label.text = game.call("get_resource_text") as String
	actions_label.text = game.call("get_selected_actions_text") as String
	name_label.text = game.call("get_selected_name") as String
	details_label.text = game.call("get_selected_details_text") as String
	_update_portrait(game.call("get_selected_portrait_path") as String)
	_update_action_button(action_button_0, 0)
	_update_action_button(action_button_1, 1)


func _update_portrait(path: String) -> void:
	if path.is_empty():
		portrait.texture = null
		return

	if not portrait_cache.has(path):
		var texture := _load_texture(path)
		if texture == null:
			portrait.texture = null
			return
		portrait_cache[path] = texture

	portrait.texture = portrait_cache[path]


func _update_action_button(button: Button, index: int) -> void:
	var text_label: Label = button.get_node("Text") as Label
	var icon_rect: TextureRect = button.get_node("Icon") as TextureRect
	text_label.text = game.call("get_action_button_text", index) as String
	icon_rect.texture = _load_icon(game.call("get_action_button_icon_path", index) as String)
	button.text = ""
	button.disabled = not (game.call("is_action_button_enabled", index) as bool)
	button.visible = not text_label.text.is_empty() or icon_rect.texture != null


func _load_icon(path: String) -> Texture2D:
	if path.is_empty():
		return null

	if not icon_cache.has(path):
		var texture := _load_texture(path)
		if texture == null:
			return null
		icon_cache[path] = texture

	return icon_cache[path]


func _load_texture(path: String) -> Texture2D:
	if ResourceLoader.exists(path, "Texture2D"):
		var texture := load(path) as Texture2D
		if texture != null:
			return texture

	var image := Image.new()
	if image.load(path) != OK:
		return null
	return ImageTexture.create_from_image(image)


func _on_action_button_pressed(index: int) -> void:
	if game == null:
		return
	game.call("perform_action_button", index)
	_update_hud()
