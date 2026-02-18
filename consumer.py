from kafka import KafkaConsumer
import json

# SOLUTION: Set a unique group_id and auto_offset_reset='earliest'
# 'earliest' forces Kafka to send all messages from the beginning of the topic.
consumer = KafkaConsumer(
    "deepstream-analytics",
    bootstrap_servers="localhost:9093",
    
    # 1. Define a Consumer Group ID
    group_id="deepstream-test-group-v2", 
    
    # 2. CRITICAL: Read from the beginning if no offset is saved
    auto_offset_reset="earliest", 
    
    # 3. Enable auto commit (saves your position)
    enable_auto_commit=True
)

print("Waiting for messages... (Press Ctrl+C to stop)")

for msg in consumer:
    print(f"Received on Partition {msg.partition}: Key={msg.key}")
    
    # Decode the message value
    try:
        data = json.loads(msg.value.decode("utf-8"))
        print(json.dumps(data, indent=2)) # Pretty print JSON
    except Exception as e:
        print(f"Error decoding: {e}")