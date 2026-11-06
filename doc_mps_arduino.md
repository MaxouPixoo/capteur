# MPS côté Arduino : glossaire de la bibliothèque

`#include <MPS.h>` fournit l'objet global `MPS` et l'espace de noms `mps`. Cible : esp32 by Espressif **3.x**.

## Nœud : `MPS` et `mps::`

| Fonction | Paramètres | Retour |
|---|---|---|
| `mps::makeSubject(name)` | `const char*` nom du subject | `mps::Subject` |
| `MPS.begin(gatewayMacAddress, deviceName, wifiChannel = 1)` | `const uint8_t[6]` MAC de la passerelle, `const char*` nom (16 car. max), `uint8_t` canal | `bool`, `false` si la radio ne démarre pas |
| `MPS.loop()` | aucun | `void` |
| `MPS.on(subject, handler, userContext = nullptr)` | `mps::Subject`, `void handler(const mps::FrameView& message, void* userContext)`, `void*` | `mps::ErrorCode` : `Ok`, `HashCollision`, `HandlerTableFull` (32 max), `QueueFull` |
| `MPS.off(subject)` | `mps::Subject` | `mps::ErrorCode` : `Ok`, `NotFound` |
| `mps::readPayloadAs(message, &output)` | `const mps::FrameView&`, pointeur vers votre structure | `bool`, `false` si la taille diffère |
| `MPS.publishValue(subject, value, requestAcknowledgement = false)` | `mps::Subject`, structure triviale (238 octets max), `bool` | `mps::ErrorCode` (voir ci-dessous) |
| `MPS.publish(subject, data, dataLength, requestAcknowledgement = false)` | `mps::Subject`, `const void*`, `size_t` (238 max), `bool` | `mps::ErrorCode` |
| `MPS.publishHeartbeat(subject, healthState = 0)` | `mps::Subject` `hb_…`, `uint8_t` 0 OK, 1 dégradé, 2 défaut | `mps::ErrorCode` |
| `MPS.isRegistered()` | aucun | `bool`, session ouverte |
| `MPS.assignedNodeId()` | aucun | `uint8_t` (1 à 239), valable une fois enregistré |
| `MPS.setRegisteredCallback(callback)` | `void callback()` | `void` |
| `MPS.setUnregisteredCallback(callback)` | `void callback()` | `void` |
| `MPS.setSendFailedCallback(callback)` | `void callback(uint16_t sequenceNumber, uint32_t subjectId, mps::SendFailureReason reason, mps::AcknowledgementStatus status)` | `void` |
| `MPS.setLogOutput(output)` | `Print*` (`&Serial`), `nullptr` pour couper | `void` |
| `MPS.counters()` | aucun | `const mps::ClientCounters&` : `retransmissionCount`, `sendFailureCount`, `duplicateCount`… |
| `MPS.radioSendFailureCount()`, `droppedReceivedFrameCount()`, `foreignFrameCount()`, `receivedFrameCount()` | aucun | `uint32_t`, diagnostic |

Champs de `mps::FrameView` : `sourceId`, `subjectId`, `payloadLength`, `payload`, `isAcknowledgementRequested()`.

Codes de publication : `Ok` (mis en file), `NotRegistered` (session fermée, rien n'est envoyé), `QueueFull` (8 messages en file), `PayloadTooBig`, `SystemSubjectForbidden` (`sys_`).

## Bonnes pratiques

- **`mps::makeSubject`** : toujours dans une variable globale `constexpr`, pour qu'un nom invalide donne une erreur de compilation (`invalidSubjectName_see_spec_3_2`). Construisez les noms depuis `DEVICE_NAME` : `mps::makeSubject("status_" DEVICE_NAME "_counter")`.
- **`MPS.begin`** : même MAC et même canal que la passerelle. Appelez `setLogOutput` avant pour voir le démarrage. Ne réactivez pas l'économie d'énergie Wi-Fi.
- **`MPS.loop`** : à chaque tour de `loop()`, au moins toutes les 10 ms. Aucun `delay()` long dans le croquis : utilisez `millis()`.
- **Handlers de `MPS.on`** : courts, sans attente. `message.payload` n'est valable que pendant l'appel.
- **`mps::readPayloadAs`** : dans chaque handler, et `return` s'il renvoie `false`. Ne lisez jamais `message.payload` directement.
- **`MPS.publishValue`** : structure `__attribute__((packed))` avec `static_assert` sur sa taille, champs à taille fixe (`uint8_t`, `int16_t`…, jamais `int`, `bool`, `enum`). Passez la valeur, pas un pointeur. ACK à `true` pour `set_` et `cmd_`, `false` pour `status_`, `get_` et `hb_`. Sur `NotRegistered`, n'insistez pas en boucle. Après une commande reçue, republiez le `status_` : c'est lui qui prouve qu'elle est appliquée.
- **`MPS.publishHeartbeat`** : appelé par votre code métier (par exemple toutes les secondes avec `millis()`), jamais ailleurs, pour que son arrêt révèle un blocage.
- **`MPS.setRegisteredCallback`** : republiez-y tout votre état courant, car la passerelle ne garde aucun message.
- **`MPS.setSendFailedCallback`** : affichage sur `Serial` uniquement, **ne publiez pas** depuis ce callback.
- **Payloads** : 238 octets max, aucun secret (rien n'est chiffré).

