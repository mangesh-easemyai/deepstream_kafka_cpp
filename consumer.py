import json
import signal
import sys
from kafka import KafkaConsumer

# Global flag for the running loop
running = True

def signal_handler(sig, frame):
    global running
    print("\nShutdown signal received, closing consumer gracefully...")
    running = False

def process_camera_stream(source_id, payload):
    """
    Routes the data based on the source_id (Partition Key).
    """
    frame_id = payload.get('task_details', {}).get('frame_id', 'N/A')
    
    if source_id == "testid1":
        print(f"[Processing] Source: {source_id} | Frame: {frame_id}")
    elif source_id == "1bfa0d514984801bb4ddbd760bfe9c5":
        print(f"[Processing] Source: {source_id} | Frame: {frame_id}")
    else:
        print(f"[Unknown Source] ID: {source_id} | Frame: {frame_id}")

def main():
    global running
    
    # Setup signal handlers for graceful shutdown (Ctrl+C)
    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)

    print("[INFO] Connecting to Kafka...")

    # Initialize Consumer
    # We do NOT use the standard 'for msg in consumer' loop because it blocks
    # and prevents the signal handler from stopping the script gracefully.
    consumer = KafkaConsumer(
        "deepstream-analytics",
        bootstrap_servers="localhost:9093",
        group_id="deepstream-analytics-processor",
        auto_offset_reset="latest",
        enable_auto_commit=True,
        # consumer_timeout_ms allows the loop to break periodically 
        # to check if 'running' is still True.
        consumer_timeout_ms=1000 
    )

    print(f"[INFO] Subscribed to topic: deepstream-analytics. Waiting for frames...")

    try:
        while running:
            # This polls for 1 second. If no messages, it continues loop.
            # This allows the 'running' flag to be checked.
            raw_msgs = consumer.poll(timeout_ms=1000)
            
            if not raw_msgs:
                continue

            for tp, msgs in raw_msgs.items():
                for msg in msgs:
                    try:
                        # --- 1. Get the Partition Key (source_id) ---
                        # In kafka-python, msg.key is a property, not a function
                        raw_key = msg.key
                        print(raw_key)
                        source_id = raw_key.decode('utf-8') if raw_key else "UNKNOWN"

                        # --- 2. Get the JSON Payload ---
                        raw_value = msg.value
                        
                        if raw_value is None:
                            print("[WARN] Received message with NULL value")
                            continue

                        # Decode bytes to string
                        json_str = raw_value.decode('utf-8')
                        
                        # CRITICAL: DeepStream often sends Null Terminators (\x00)
                        # We must strip them before parsing JSON
                        json_str = json_str.strip().rstrip('\x00')
                        
                        # Parse JSON
                        payload = json.loads(json_str)

                        # --- 3. Process Data ---
                        process_camera_stream(source_id, payload)

                    except json.JSONDecodeError as e:
                        print(f"[ERROR] Malformed JSON on partition {msg.partition}: {e}")
                    except Exception as e:
                        print(f"[ERROR] Processing failure: {str(e)}")

    finally:
        # Graceful Teardown
        consumer.close()
        print("[INFO] Consumer closed. Exiting.")

if __name__ == '__main__':
    main()