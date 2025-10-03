import paho.mqtt.client as mqtt

# Define the callback for when a message is received
def on_message(client, userdata, msg):
    topic = msg.topic
    payload = msg.payload.decode()
    
    
    # Simulate different actions based on the message
    if payload == "ACTION1":
        print(f"[{topic}] Interrupt received: Do action 1")
    elif payload == "ACTION2":
        print(f"[{topic}] Interrupt received: Do action 2")
    else:
        print(f"[{topic}] Received unknown message: {payload}")

# Setup MQTT client
client = mqtt.Client()
client.on_message = on_message

# Connect to your Raspberry Pi MQTT broker
broker_ip = "192.168.1.16"  # replace with your Pi's IP
client.connect(broker_ip, 1883, 60)

# Subscribe to a topic
client.subscribe("test/topic")
client.subscribe("sensors/gnss")

# Blocking loop to process network traffic and callbacks
print("Waiting for messages... Press Ctrl+C to exit")
client.loop_forever()
