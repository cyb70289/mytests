import io.opentelemetry.api.OpenTelemetry;
import io.opentelemetry.api.metrics.LongHistogram;
import io.opentelemetry.exporter.logging.LoggingMetricExporter;
import io.opentelemetry.sdk.OpenTelemetrySdk;
import io.opentelemetry.sdk.metrics.SdkMeterProvider;
import io.opentelemetry.sdk.metrics.export.MetricReader;
import io.opentelemetry.sdk.metrics.export.PeriodicMetricReader;
import java.time.Duration;

final class Configuration {
  public static OpenTelemetry initOpenTelemetry() {
    MetricReader periodicReader =
        PeriodicMetricReader.builder(LoggingMetricExporter.create())
            .setInterval(Duration.ofMillis(1000))
            .build();

    SdkMeterProvider meterProvider =
        SdkMeterProvider.builder().registerMetricReader(periodicReader).build();

    return OpenTelemetrySdk.builder()
        .setMeterProvider(meterProvider)
        .buildAndRegisterGlobal();
  }
}

public final class TestMeter {
  private static final String INSTRUMENTATION_NAME = TestMeter.class.getName();

  private final LongHistogram histogram;

  public TestMeter(OpenTelemetry openTelemetry) {
    histogram = openTelemetry.getMeter(INSTRUMENTATION_NAME).histogramBuilder("HISTOGRAM").ofLongs().build();
  }

  // wrap original method
  private void doWork() {
    long startTime = System.currentTimeMillis();

    _doWork();

    long duration = System.currentTimeMillis() - startTime;
    histogram.record(duration);
  }

  // sleep 500 ~ 1500ms
  private void _doWork() {
    try {
      Thread.sleep(500 + new java.util.Random().nextInt(1000));
    } catch (InterruptedException e) {
    }
  }

  public static void main(String[] args) {
    OpenTelemetry oTel = Configuration.initOpenTelemetry();

    TestMeter tester = new TestMeter(oTel);
    while (true) {
      tester.doWork();
    }
  }
}
