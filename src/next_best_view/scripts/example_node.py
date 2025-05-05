#!/usr/bin/env python3
import rclpy
from rclpy.node import Node

class ExampleNode(Node):
    def __init__(self):
        super().__init__("my_example_node")


def main(args=None):
    rclpy.init(args=args)

    node = ExampleNode()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()
