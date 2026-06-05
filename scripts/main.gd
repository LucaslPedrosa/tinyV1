extends Node2D

var game: Node
var portrait_cache: Dictionary[String, Texture2D] = {}
var icon_cache: Dictionary[String, Texture2D] = {}

@onready var resources_label: Label = $HudLayer/BottomHud/ResourcesBox/ResourcesLabel
@onready var actions_label: Label = $HudLayer/BottomHud/ActionsBox/ActionsLabel
@onready var action_button_0: Button = $HudLayer/BottomHud/ActionsBox/ActionButton0
@onready var action_button_1: Button = $HudLayer/BottomHud/ActionsBox/ActionButton1
@onready var portrait: TextureRect = $HudLayer/BottomHud/SelectionBox/Portrait
@onready var name_label: Label = $HudLayer/BottomHud/SelectionBox/NameLabel
@onready var details_label: Label = $HudLayer/BottomHud/SelectionBox/DetailsLabel


func _ready() -> void:
	action_button_0.pressed.connect(_on_action_button_pressed.bind(0))
	action_button_1.pressed.connect(_on_action_button_pressed.bind(1))

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
	if game != null:
		_update_hud()


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
		var image := Image.new()
		var error := image.load(path)
		if error != OK:
			portrait.texture = null
			return
		portrait_cache[path] = ImageTexture.create_from_image(image)

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
		var image := Image.new()
		var error := image.load(path)
		if error != OK:
			return null
		icon_cache[path] = ImageTexture.create_from_image(image)

	return icon_cache[path]


func _on_action_button_pressed(index: int) -> void:
	if game == null:
		return
	game.call("perform_action_button", index)
	_update_hud()
