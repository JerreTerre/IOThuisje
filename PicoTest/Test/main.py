import bluetooth
from micropython import const

# BLE events
_IRQ_CENTRAL_CONNECT = const(1)
_IRQ_CENTRAL_DISCONNECT = const(2)
_IRQ_GATTS_WRITE = const(3)

# Nordic UART Service UUIDs (RoboRemo compatible)
UART_UUID = bluetooth.UUID("6E400001-B5A3-F393-E0A9-E50E24DCCA9E")
UART_TX   = bluetooth.UUID("6E400003-B5A3-F393-E0A9-E50E24DCCA9E")
UART_RX   = bluetooth.UUID("6E400002-B5A3-F393-E0A9-E50E24DCCA9E")

UART_SERVICE = (
    UART_UUID,
    (
        (UART_TX, bluetooth.FLAG_NOTIFY),
        (UART_RX, bluetooth.FLAG_WRITE),
    ),
)

ble = bluetooth.BLE()
ble.active(True)

def bt_irq(event, data):
    if event == _IRQ_CENTRAL_CONNECT:
        print("✅ Connected")

    elif event == _IRQ_CENTRAL_DISCONNECT:
        print("❌ Disconnected")
        advertise()

    elif event == _IRQ_GATTS_WRITE:
        _, value_handle = data
        if value_handle == rx_handle:
            msg = ble.gatts_read(rx_handle).decode().strip()
            print("RX:", msg)

ble.irq(bt_irq)

((tx_handle, rx_handle),) = ble.gatts_register_services((UART_SERVICE,))

def advertise():
    ble.gap_advertise(
        100_000,
        b"\x02\x01\x06" + b"\x0A\x09PicoBLE"
    )

advertise()
print("🔵 BLE test ready")