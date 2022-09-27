package test.kotlin.integration.proxying


import android.content.Context
import android.net.ConnectivityManager
import android.net.ProxyInfo
import androidx.test.core.app.ApplicationProvider

import io.envoyproxy.envoymobile.LogLevel
import io.envoyproxy.envoymobile.Custom
import io.envoyproxy.envoymobile.Engine
import io.envoyproxy.envoymobile.UpstreamHttpProtocol
import io.envoyproxy.envoymobile.AndroidEngineBuilder
import io.envoyproxy.envoymobile.RequestHeadersBuilder
import io.envoyproxy.envoymobile.RequestMethod
import io.envoyproxy.envoymobile.ResponseHeaders
import io.envoyproxy.envoymobile.StreamIntel
import io.envoyproxy.envoymobile.engine.JniLibrary

import java.util.concurrent.CountDownLatch
import java.util.concurrent.Executors
import java.util.concurrent.TimeUnit

import org.assertj.core.api.Assertions.assertThat
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.Mock
import org.mockito.Mockito
import org.robolectric.RobolectricTestRunner

//                                                ┌──────────────────┐
//                                                │   Proxy Engine   │
//                                                │ ┌──────────────┐ │
// ┌─────────────────────────┐                  ┌─┼─►listener_proxy│ │
// │https://api.lyft.com/ping│  ┌──────────────┬┘ │ └──────┬───────┘ │ ┌────────────┐
// │         Request         ├──►Android Engine│  │        │         │ │api.lyft.com│
// └─────────────────────────┘  └──────────────┘  │ ┌──────▼──────┐  │ └──────▲─────┘
//                                                │ │cluster_proxy│  │        │
//                                                │ └─────────────┴──┼────────┘
//                                                │                  │
//                                                └──────────────────┘
@RunWith(RobolectricTestRunner::class)
class PerformHTTPSRequestBadHostname {
  init {
    JniLibrary.loadTestLibrary()
  }

  @Test
  fun `attempts an HTTPs request through a proxy using an async DNS resolution that fails`() {
    val port = (10001..11000).random()

    val mockContext = Mockito.mock(Context::class.java)
    Mockito.`when`(mockContext.getApplicationContext()).thenReturn(mockContext)
    val mockConnectivityManager = Mockito.mock(ConnectivityManager::class.java)
    Mockito.`when`(mockContext.getSystemService(Mockito.anyString())).thenReturn(mockConnectivityManager)
    Mockito.`when`(mockConnectivityManager.getDefaultProxy()).thenReturn(ProxyInfo.buildDirectProxy("loopback", port))

    val onEngineRunningLatch = CountDownLatch(1)
    val onProxyEngineRunningLatch = CountDownLatch(1)
    val onErrorLatch = CountDownLatch(1)

    val proxyEngineBuilder = Proxy(ApplicationProvider.getApplicationContext(), port).https()
    val proxyEngine = proxyEngineBuilder
      .addLogLevel(LogLevel.DEBUG)
      .addDNSQueryTimeoutSeconds(2)
      .setOnEngineRunning { onProxyEngineRunningLatch.countDown() }
      .build()

    onProxyEngineRunningLatch.await(10, TimeUnit.SECONDS)
    assertThat(onProxyEngineRunningLatch.count).isEqualTo(0)

    val builder = AndroidEngineBuilder(mockContext)
    val engine = builder
      .addLogLevel(LogLevel.DEBUG)
      .enableProxying(true)
      .setOnEngineRunning { onEngineRunningLatch.countDown() }
      .build()

    onEngineRunningLatch.await(10, TimeUnit.SECONDS)
    assertThat(onEngineRunningLatch.count).isEqualTo(0)

    val requestHeaders = RequestHeadersBuilder(
      method = RequestMethod.GET,
      scheme = "https",
      authority = "api.lyft.com",
      path = "/ping"
    )
      .build()

    engine
      .streamClient()
      .newStreamPrototype()
      .setOnError { _, _ ->
        onErrorLatch.countDown()
      }
      .start(Executors.newSingleThreadExecutor())
      .sendHeaders(requestHeaders, true)

    onErrorLatch.await(15, TimeUnit.SECONDS)
    assertThat(onErrorLatch.count).isEqualTo(0)

    engine.terminate()
    proxyEngine.terminate()
  }
}