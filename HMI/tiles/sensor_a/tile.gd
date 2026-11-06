extends Tile

# ---- CONSTANTES ----

const DEVICE_NAME = "sensor_a"
const SUB_HB = "hb_" + DEVICE_NAME
## Délai sans heartbeat au-delà duquel le capteur est considéré comme bloqué (3,5 périodes de 1 s).
const HB_TIMEOUT_S := 3.5
## Durée d'allumage de l'icône de heartbeat à chaque réception.
const HB_BLINK_S := 0.5

const SUB_STATUS_COUNTER = "status_" + DEVICE_NAME + "_counter"
const SUB_STATUS_TEMPERATURE = "status_" + DEVICE_NAME + "_temperature"
const SUB_STATUS_HUMIDITE = "status_" + DEVICE_NAME + "_humidite"
const SUB_CMD_RESET = "cmd_" + DEVICE_NAME + "_reset"
const SUB_CMD_LED = "cmd_" + DEVICE_NAME + "_led"

# ---- VARIABLES ----
var _node_id: int = -1
var _seconds_since_hb := INF

var _connected: bool = false:
	set(value):
		if value == _connected: return
		_connected = value
		if _connected:
			_on_sensor_connected()
		else:
			_on_sensor_disconnected()

# ---- FONCTIONS ----

func _ready() -> void:
	super() 
	
	var reset_button := find_child("ResetButton", true, false) as Button

	if reset_button != null:
		reset_button.pressed.connect(_on_reset_pressed)
	else:
		push_warning("sensor_a : bouton ResetButton introuvable")
	
	var led_picker := find_child("LedColorPicker", true, false) as ColorPickerButton
	
	if led_picker:
		led_picker.color_changed.connect(_on_led_color_changed)
func _on_reset_pressed() -> void:
	publish(SUB_CMD_RESET, PackedByteArray(), true)

func get_subscriptions() -> PackedStringArray:
	var subscriptions: PackedStringArray = []
	subscriptions.append(SUB_HB)
	subscriptions.append(SUB_STATUS_COUNTER)
	subscriptions.append(SUB_STATUS_TEMPERATURE)
	subscriptions.append(SUB_STATUS_HUMIDITE)
	return subscriptions


func on_node_online(node_id: int, _mac_address: String, device_name: String) -> void:
	if device_name != DEVICE_NAME: return
	_node_id = node_id 
	%IDLabel.text = "node_id: %d" % _node_id


func on_node_offline(node_id: int, _reason_code: int) -> void:
	if node_id == _node_id: _connected = false


func on_gateway_reachability_changed(is_gateway_reachable: bool) -> void:
	if is_gateway_reachable: return
	_connected = false
	_node_id = -1  


func on_message(subject_name: String, source_id: int, payload: PackedByteArray) -> void:
	print("----- MESSAGE RECU -----")
	print("Subject : ", subject_name)
	print("Source ID : ", source_id)
	print("Node ID attendu : ", _node_id)
	print("Taille payload : ", payload.size())
	if subject_name == SUB_HB:
		_on_hb(source_id)

	if source_id != _node_id:
		print("Message ignore : mauvais node_id")
		return

	if subject_name == SUB_STATUS_COUNTER:
		print("Message compteur detecte !")

		if payload.size() != 4:
			print("ERREUR : taille incorrecte")
			return

		var compteur = payload.decode_u32(0)
		print("Valeur compteur : ", compteur)
		
		%CounterLabel.text = "Compteur : %d" % compteur

	if subject_name == SUB_STATUS_TEMPERATURE:

		if payload.size() != 4:
			print("Erreur : payload temperature incorrect")
			return

		var temp = payload.decode_float(0)

		print("Temperature recue : ", temp)

		%TemperatureLabel.text = "Température : %.1f °C" % temp
		
	if subject_name == SUB_STATUS_HUMIDITE:

		if payload.size() != 4:
			print("Erreur : payload humidite incorrect")
			return

		var hum = payload.decode_float(0)

		print("Humidite recue : ", hum)

		%HumiditeLabel.text = "Humidité : %.1f %%" % hum

func _process(delta: float) -> void:
	_seconds_since_hb += delta
	%HeartbeatIcon.state = _seconds_since_hb < HB_BLINK_S
	if _connected and _seconds_since_hb >= HB_TIMEOUT_S:
		_connected = false 

func _on_sensor_connected():
	%IDLabel.text = "node_id: %d" % _node_id
	%ConnectedIcon.state = true

func _on_sensor_disconnected():
	%IDLabel.text = "node_id: ∅"
	%ConnectedIcon.state = false

func _on_hb(source_id: int):
	if _node_id == -1: _node_id = source_id
	if source_id != _node_id: return
	_seconds_since_hb = 0.0
	_connected = true

func _on_led_color_changed(color: Color) -> void:
	var r := roundi(color.r * 255.0)
	var g := roundi(color.g * 255.0)
	var b := roundi(color.b * 255.0)

	var payload := PackedByteArray([r, g, b])

	publish(SUB_CMD_LED, payload, true)

	print("LED RGB : ", r, ", ", g, ", ", b)
