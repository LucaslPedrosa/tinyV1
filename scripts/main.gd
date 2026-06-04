extends Node2D

var game: Node

@onready var status_label: Label = $HudLayer/Label
@onready var resources_label: Label = $HudLayer/BottomHud/ResourcesBox/ResourcesLabel
@onready var actions_label: Label = $HudLayer/BottomHud/ActionsBox/ActionsLabel
@onready var action_button_0: Button = $HudLayer/BottomHud/ActionsBox/ActionButton0
@onready var action_button_1: Button = $HudLayer/BottomHud/ActionsBox/ActionButton1
@onready var portrait: ColorRect = $HudLayer/BottomHud/SelectionBox/Portrait
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
		status_label.text = "tinyV1 project loaded; build the C++ extension next"


func _process(_delta: float) -> void:
	if game != null:
		_update_hud()


func _update_hud() -> void:
	status_label.text = game.call("get_status_text") as String
	resources_label.text = game.call("get_resource_text") as String
	actions_label.text = game.call("get_selected_actions_text") as String
	name_label.text = game.call("get_selected_name") as String
	details_label.text = game.call("get_selected_details_text") as String
	portrait.color = game.call("get_selected_portrait_color") as Color
	_update_action_button(action_button_0, 0)
	_update_action_button(action_button_1, 1)


func _update_action_button(button: Button, index: int) -> void:
	button.text = game.call("get_action_button_text", index) as String
	button.disabled = not (game.call("is_action_button_enabled", index) as bool)
	button.visible = not button.text.is_empty()


func _on_action_button_pressed(index: int) -> void:
	if game == null:
		return
	game.call("perform_action_button", index)
	_update_hud()
