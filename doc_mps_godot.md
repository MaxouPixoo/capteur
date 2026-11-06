# MPS côté Godot : glossaire de la classe `Tile`

Votre tuile commence par `extends Tile`. Les méthodes marquées *(à vérifier)* sont déduites des exemples, pas lues dans `core/tile.gd`.

## Méthodes à redéfinir

Toutes sont appelées sur le thread principal : on peut y modifier l'interface.

| Méthode | Paramètres | Rôle |
|---|---|---|
| `get_subscriptions() -> PackedStringArray` | aucun | Liste des subjects reçus par la tuile |
| `on_message(subject_name, source_id, payload) -> void` | `String` nom du subject, `int` node_id de l'émetteur, `PackedByteArray` octets bruts | Un message est arrivé sur un subject abonné |
| `on_node_online(node_id, mac_address, device_name) -> void` | `int`, `String`, `String` | Un appareil (n'importe lequel) s'est enregistré |
| `on_node_offline(node_id, reason_code) -> void` | `int`, `int` | Un appareil est hors ligne |
| `on_gateway_reachability_changed(is_gateway_reachable) -> void` | `bool` | La passerelle devient joignable ou non |
| `_ready() -> void` | aucun | Initialisation de la tuile |

## Méthodes et outils à appeler

| Élément | Paramètres | Rôle |
|---|---|---|
| `publish(subject_name, payload, ack_requested = false)` *(à vérifier)* | `String`, `PackedByteArray`, `bool` | Envoie un message (mis en file, retour immédiat) |
| `MPSHelpers.decode_heartbeat(payload) -> Dictionary` | `PackedByteArray` | Décode un `hb_` ; renvoie `{}` si la taille est fausse |

## Bonnes pratiques

- **`_ready()`** : première ligne `super()`. Sans lui, aucun abonnement et la tuile reste vide sans erreur. Même règle pour `_exit_tree()` *(à vérifier)*.
- **`get_subscriptions()`** : un subject absent de la liste n'arrive jamais dans `on_message`. Construisez les noms depuis `DEVICE_NAME` (`"status_" + DEVICE_NAME + "_counter"`), qui doit être identique au `config.h`.
- **`on_message()`** : ignorez ce qui ne vient pas de votre capteur (`if source_id != _node_id: return`), puis **vérifiez `payload.size()` avant de décoder** et ignorez le message si elle est fausse. Lecture avec `decode_u8/s8/u16/s16/u32/s32/float(offset)`, en petit-boutiste comme l'ESP32.
- **`on_node_online()`** : filtrez sur `device_name == DEVICE_NAME` et mémorisez `node_id`, qui change après un redémarrage. Bon endroit pour envoyer un `get_`.
- **`on_gateway_reachability_changed(false)`** : remettez `_node_id = -1`, la passerelle peut redistribuer les identifiants.
- **`publish()`** : ACK à `true` pour `cmd_` et `set_`, `false` pour `get_`. Après un clic, n'écrivez rien à l'écran : attendez le `status_` du capteur, seule preuve que l'ordre a été appliqué.

## Conventions de nommage

- Subjects : minuscules `a-z 0-9`, mots séparés par **un seul** `_`, 64 caractères au plus, préfixe `hb_`, `status_`, `cmd_`, `set_`, `get_` ou `evt_` (`sys_` réservé).
- `_` en tête d'un **paramètre** (`_payload`) : paramètre volontairement inutilisé, retirez-le dès que vous l'utilisez. En tête d'une **variable ou fonction** (`_node_id`, `_on_hb`) : interne à la tuile. `_ready`, `_process` : callbacks appelés par Godot.
