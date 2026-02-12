import json
import base64
import os
from confluent_kafka import Consumer, KafkaError

# --- Configuration ---
KAFKA_TOPIC = "deepstream-kafka-python"
KAFKA_BROKER = "localhost:9093"
OUTPUT_DIR = "received_frames"

# Create output directory if it doesn't exist
if not os.path.exists(OUTPUT_DIR):
    os.makedirs(OUTPUT_DIR)

# --- Consumer Setup ---
conf = {
    'bootstrap.servers': KAFKA_BROKER,
    'group.id': 'deepstream-image-viewer-group',
    'auto.offset.reset': 'latest' # Start reading from new messages
}

consumer = Consumer(conf)
consumer.subscribe([KAFKA_TOPIC])

print(f"📡 Listening to topic: {KAFKA_TOPIC}...")
print(f"📂 Images will be saved to: {OUTPUT_DIR}/")

try:
    while True:
        # Poll for messages (timeout 1.0s)
        msg = consumer.poll(1.0)

        if msg is None:
            continue
        if msg.error():
            if msg.error().code() == KafkaError._PARTITION_EOF:
                continue
            else:
                print(msg.error())
                break

        # --- Process Message ---
        try:
            # 1. Decode JSON
            payload = msg.value().decode('utf-8')
            print(payload)
            data = json.loads(payload)
            print(payload)
            # 2. Extract Info
            camera_id = data.get("camera_id", "unknown")
            timestamp = data.get("timestamp", "no_time")
            rtsp_url = data.get("rtsp_url", "")
            base64_img = data.get("frame_image_base64", None)

            print(f"✅ Received: Cam: {camera_id} | Time: {timestamp} | Size: {len(payload)} bytes")

            # 3. Save Image
            if base64_img:
                # Clean timestamp for filename
                safe_time = timestamp.replace(":", "-").replace(".", "_")
                filename = f"{OUTPUT_DIR}/{camera_id}_{safe_time}.jpg"

                # Decode Base64 -> Binary -> File
                with open(filename, "wb") as f:
                    f.write(base64.b64decode(base64_img))
                # print(f"   └── Saved image: {filename}")

        except Exception as e:
            print(f"❌ Error parsing message: {e}")

except KeyboardInterrupt:
    print("\n🛑 Stopping consumer...")
finally:
    consumer.close()