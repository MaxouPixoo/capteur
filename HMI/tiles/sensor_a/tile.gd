extends Tile

const DEVICE_NAME = "sensor_a"
const SUB_HB = "hb_" + DEVICE_NAME
const SUB_STATUS_COUNTER = "status_" + DEVICE_NAME + "_counter"
const SUB_STATUS_TEMPERATURE = "status_" + DEVICE_NAME + "_temperature"
const SUB_STATUS_HUMIDITE = "status_" + DEVICE_NAME + "_humidite"
const SUB_STATUS_LED = "status_" + DEVICE_NAME + "_led"
const SUB_CMD_RESET = "cmd_" + DEVICE_NAME + "_reset"
const SUB_SET_LED_COLOR = "set_" + DEVICE_NAME + "_led_color"
const SUB_SET_LED_ENABLED = "set_" + DEVICE_NAME + "_led_enabled"
const HB_TIMEOUT_S := 3.5
const HB_BLINK_S := 0.5
const PAYLOAD_SIZE_4 := 4
const OFFSET_FIRST_BYTE := 0
const OFFSET_GREEN_BYTE := 1
const OFFSET_BLUE_BYTE := 2
const OFFSET_LED_ENABLED_BYTE := 3
const LED_ENABLED_VALUE := 1
const RGB_MAX := 255.0
const DISCONNECTED_NODE_ID := -1
const DISCONNECTED_COLOR := Color(0.5, 0.5, 0.5, 1.0)
const CONNECTED_COLOR := Color.WHITE

var _node_id: int = DISCONNECTED_NODE_ID
var _seconds_since_hb := INF
var _led_enabled := false
var _updating_led_controls := false

@onready var _reset_button: Button = %ResetButton
@onready var _led_color_picker: ColorPickerButton = %LedColorPicker
@onready var _led_toggle_button: Button = %LedToggleButton
@onready var _value_labels: Array[Label] = [%CounterLabel, %TemperatureLabel, %HumiditeLabel]

var _connected: bool = false:
	set(value):
		if value == _connected:
			return
		_connected = value
		if _connected:
			_on_sensor_connected()
		else:
			_on_sensor_disconnected()


func _ready() -> void:
	super()
	_reset_button.pressed.connect(_on_reset_pressed)
	_led_color_picker.color_changed.connect(_on_led_color_changed)
	_led_toggle_button.pressed.connect(_on_led_toggle_pressed)
	_set_commands_enabled(false)
	_set_values_fresh(false)


func _on_reset_pressed() -> void:
	if _connected:
		publish(SUB_CMD_RESET, PackedByteArray(), true)


func get_subscriptions() -> PackedStringArray:
	return PackedStringArray([
		SUB_HB,
		SUB_STATUS_COUNTER,
		SUB_STATUS_TEMPERATURE,
		SUB_STATUS_HUMIDITE,
		SUB_STATUS_LED,
	])


func on_node_online(node_id: int, _mac_address: String, device_name: String) -> void:
	if device_name != DEVICE_NAME:
		return
	_node_id = node_id
	%IDLabel.text = "node_id: %d" % _node_id


func on_node_offline(node_id: int, _reason_code: int) -> void:
	if node_id == _node_id:
		_connected = false


func on_gateway_reachability_changed(is_gateway_reachable: bool) -> void:
	if is_gateway_reachable:
		return
	_connected = false
	_node_id = DISCONNECTED_NODE_ID


func on_message(subject_name: String, source_id: int, payload: PackedByteArray) -> void:
	if subject_name == SUB_HB:
		_on_hb(source_id)
	if source_id != _node_id:
		return

	if subject_name == SUB_STATUS_COUNTER:
		if payload.size() != PAYLOAD_SIZE_4:
			return
		%CounterLabel.text = "Compteur : %d" % payload.decode_u32(OFFSET_FIRST_BYTE)
		_set_label_fresh(%CounterLabel)
	elif subject_name == SUB_STATUS_TEMPERATURE:
		if payload.size() != PAYLOAD_SIZE_4:
			return
		var temperature := payload.decode_float(OFFSET_FIRST_BYTE)
		%TemperatureLabel.text = "Température : %.1f °C" % temperature
		_set_label_fresh(%TemperatureLabel)
	elif subject_name == SUB_STATUS_HUMIDITE:
		if payload.size() != PAYLOAD_SIZE_4:
			return
		var humidity := payload.decode_float(OFFSET_FIRST_BYTE)
		%HumiditeLabel.text = "Humidité : %.1f %%" % humidity
		_set_label_fresh(%HumiditeLabel)
	elif subject_name == SUB_STATUS_LED:
		if payload.size() != PAYLOAD_SIZE_4:
			return
		_led_enabled = payload.decode_u8(OFFSET_LED_ENABLED_BYTE) == LED_ENABLED_VALUE
		var color := Color(
			payload.decode_u8(OFFSET_FIRST_BYTE) / RGB_MAX,
			payload.decode_u8(OFFSET_GREEN_BYTE) / RGB_MAX,
			payload.decode_u8(OFFSET_BLUE_BYTE) / RGB_MAX
		)
		_updating_led_controls = true
		_led_color_picker.color = color
		_led_toggle_button.text = "Éteindre la LED" if _led_enabled else "Allumer la LED"
		_updating_led_controls = false


func _process(delta: float) -> void:
	_seconds_since_hb += delta
	%HeartbeatIcon.state = _seconds_since_hb < HB_BLINK_S
	if _connected and _seconds_since_hb >= HB_TIMEOUT_S:
		_connected = false


func _on_sensor_connected() -> void:
	%IDLabel.text = "node_id: %d" % _node_id
	%ConnectedIcon.state = true
	_set_commands_enabled(true)


func _on_sensor_disconnected() -> void:
	%IDLabel.text = "node_id: ∅"
	%ConnectedIcon.state = false
	_set_commands_enabled(false)
	_set_values_fresh(false)


func _on_hb(source_id: int) -> void:
	if _node_id == DISCONNECTED_NODE_ID:
		_node_id = source_id
	if source_id != _node_id:
		return
	_seconds_since_hb = 0.0
	_connected = true


func _on_led_color_changed(color: Color) -> void:
	if not _connected or _updating_led_controls:
		return
	var payload := PackedByteArray([
		roundi(color.r * RGB_MAX),
		roundi(color.g * RGB_MAX),
		roundi(color.b * RGB_MAX),
	])
	publish(SUB_SET_LED_COLOR, payload, true)


func _on_led_toggle_pressed() -> void:
	if not _connected:
		return
	var payload := PackedByteArray([0 if _led_enabled else LED_ENABLED_VALUE])
	publish(SUB_SET_LED_ENABLED, payload, true)


func _set_commands_enabled(enabled: bool) -> void:
	_reset_button.disabled = not enabled
	_led_color_picker.disabled = not enabled
	_led_toggle_button.disabled = not enabled


func _set_values_fresh(fresh: bool) -> void:
	for label in _value_labels:
		_set_label_fresh(label, fresh)


func _set_label_fresh(label: Label, fresh: bool = true) -> void:
	label.modulate = CONNECTED_COLOR if fresh else DISCONNECTED_COLOR
