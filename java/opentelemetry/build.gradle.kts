plugins {
  id("java")
  application
}

sourceSets {
  main {
    java.setSrcDirs(setOf("."))
  }
}

repositories {
  mavenCentral()
}

dependencies {
  implementation(platform("io.opentelemetry:opentelemetry-bom:1.38.0"))
  implementation("io.opentelemetry:opentelemetry-api")
  implementation("io.opentelemetry:opentelemetry-sdk")
  implementation("io.opentelemetry:opentelemetry-exporter-logging")
}

application {
  mainClass = "TestMeter"
}
