Selim Djaafri et Maxence Vechambre

# Contrat des messages MPS

Les subjects sont construits à partir de `DEVICE_NAME` (`sensor_a`). Les payloads multioctets sont encodés en little-endian. Les structures C++ sont `packed` et leur taille est vérifiée à la compilation.

| Subject | Sens et rôle | Champs du payload |
|---|---|---|
| `hb_sensor_a` | Capteur → IHM : heartbeat périodique | `healthState` : `uint8_t`, état de santé (0 = OK), 1 octet. |
| `status_sensor_a_counter` | Capteur → IHM : valeur du compteur | `value` : `uint32_t`, sans unité, 4 octets. |
| `cmd_sensor_a_reset` | IHM → capteur : remise à zéro du compteur | Aucun champ, payload vide (0 octet). |
| `status_sensor_a_temperature` | Capteur → IHM : température mesurée | `temperature` : `float`, degrés Celsius (°C), 4 octets. |
| `status_sensor_a_humidite` | Capteur → IHM : humidité mesurée | `humidity` : `float`, pourcentage (%), 4 octets. |
| `set_sensor_a_led_color` | IHM → capteur : réglage de la couleur et allumage de la LED | `red`, `green`, `blue` : trois `uint8_t`, intensité RGB de 0 à 255 (sans unité), 1 octet chacun, 3 octets au total. |
| `set_sensor_a_led_enabled` | IHM → capteur : allumage ou extinction de la LED | `enabled` : `uint8_t`, 0 = éteinte, 1 = allumée (sans unité), 1 octet. |
| `status_sensor_a_led` | Capteur → IHM : état appliqué à la LED | `red`, `green`, `blue` : `uint8_t`, intensité RGB de 0 à 255 (sans unité), 1 octet chacun ; `enabled` : `uint8_t`, 0 = éteinte, 1 = allumée (sans unité), 1 octet. 4 octets au total. |
