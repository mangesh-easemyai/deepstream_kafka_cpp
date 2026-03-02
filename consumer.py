from kafka import KafkaConsumer
import json

consumer = KafkaConsumer(
    "deepstream-analytics",
    bootstrap_servers="localhost:9093",
    
   
)

print("Waiting for messages... (Press Ctrl+C to stop)")

for msg in consumer:
    try:
        # 1. Decode bytes to string
        json_str = msg.value.decode("utf-8")
        
        # 2. Strip whitespace and specifically the Null Terminator (\x00)
        json_str = json_str.strip().rstrip('\x00')
        
        # 3. Parse
        data = json.loads(json_str)
        
        # # 4. Print nicely
        # print(f"Frame: {data.get('task_details', {}).get('frame_id')}, "
        #       f"Detections: {len(data.get('task_details', {}).get('detections', []))}")
        
        # Optional: Print full JSON
        print(json.dumps(data, indent=2))
        # with open("logs.json","a") as f:
        #     json.dump(data,f,indent=2)

    except Exception as e:
        print(f"Error decoding: {e}")
        print(f"Raw value: {msg.value}")