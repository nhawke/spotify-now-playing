package main

import (
	"bufio"
	"context"
	"encoding/json"
	"errors"
	"flag"
	"fmt"
	"io"
	"net/http"
	"os"
	"strings"
	"time"
	"unicode"

	"github.com/google/uuid"
	"github.com/zmb3/spotify/v2"
	spotifyauth "github.com/zmb3/spotify/v2/auth"
	"go.bug.st/serial"

	"golang.org/x/oauth2"
	"golang.org/x/text/runes"
	"golang.org/x/text/transform"
	"golang.org/x/text/unicode/norm"
)

const (
	httpPort         = 8888
	tokenFile        = "access_token"
	clientIDFile     = "client_id"
	clientSecretFile = "client_secret"
)

var (
	portName string
)

func init() {
	flag.StringVar(&portName, "port", "", "Path to the serial port for communication with display. If empty, uses stdin/out")
}

func main() {
	flag.Parse()
	ctx := context.Background()

	clientID, err := os.ReadFile(clientIDFile)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Can't read client ID file: %v\n", err)
		return
	}
	clientSecret, err := os.ReadFile(clientSecretFile)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Can't read client secret file: %v\n", err)
		return
	}

	redirectURL := fmt.Sprintf("http://localhost:%v/callback", httpPort)
	auth := spotifyauth.New(
		spotifyauth.WithClientID(strings.TrimSpace(string(clientID))),
		spotifyauth.WithClientSecret(strings.TrimSpace(string(clientSecret))),
		spotifyauth.WithRedirectURL(redirectURL),
		spotifyauth.WithScopes(spotifyauth.ScopeUserReadCurrentlyPlaying),
	)

	rt, err := os.ReadFile(tokenFile)
	if err != nil && !errors.Is(err, os.ErrNotExist) {
		fmt.Fprintf(os.Stderr, "Error reading refresh token file: %v\n", err)
	}
	var token *oauth2.Token
	if len(rt) == 0 {
		// There's no previously-saved token, launch a server to collect an access code.
		state := uuid.NewString()
		url := auth.AuthURL(state)
		fmt.Fprintf(os.Stderr, "GO HERE TO AUTHENTICATE: \n%v\n\n", url)

		tc := make(chan string)
		http.HandleFunc("/callback", func(w http.ResponseWriter, r *http.Request) {
			io.WriteString(w, "Successfully authenticated! You can close this tab now.")
			tc <- r.URL.Query().Get("code")
		})

		srv := http.Server{
			Addr: fmt.Sprintf(":%d", httpPort),
		}
		go srv.ListenAndServe()

		// Ensure we get the code before closing the server.
		code := <-tc
		srv.Close()

		if code == "" {
			fmt.Fprintf(os.Stderr, "NO CODE AHH")
			return
		}
		token, err = auth.Exchange(ctx, code)
		if err != nil {
			fmt.Fprintf(os.Stderr, "Error Exchanging code: %v", err)
			return
		}
	} else {
		token = new(oauth2.Token)
		if err := json.Unmarshal(rt, token); err != nil {
			fmt.Fprintln(os.Stderr, err)
			return
		}
	}

	client := spotify.New(auth.Client(ctx, token))
	if err := wrtieClientTokenToFile(client, tokenFile); err != nil {
		fmt.Fprintln(os.Stderr, err)
		return
	}
	printInfo(ctx, client)
}

func printInfo(ctx context.Context, client *spotify.Client) {
	var portWriter io.Writer = os.Stdout
	var portReader io.Reader = os.Stdin
	delim := "\n"
	if portName != "" {
		mode := &serial.Mode{
			BaudRate: 9600,
			// DataBits: 8,
			// Parity:   serial.NoParity,
			// StopBits: 1,
		}
		p, err := serial.Open(portName, mode)
		if err != nil {
			fmt.Fprintf(os.Stderr, "Cannot open serial port %v: %v\n", portName, err)
			return
		}
		portWriter = io.MultiWriter(p, os.Stderr) // duplicate serial out to stderr for debugging
		portReader = p
		delim = fmt.Sprintf("%c", 0x4)
	}

	req := make(chan bool)
	defer close(req)
	go func() {
		s := bufio.NewScanner(portReader)
		for s.Scan() {
			got := s.Text()
			if got == "" {
				continue
			}
			fmt.Fprintf(os.Stderr, "GOT: %v\n", got)
			if got == "READY" {
				req <- true
			}
		}
	}()

	ticker := time.NewTicker(time.Second)
	defer ticker.Stop()

	info := ""

	for {
		select {
		case <-ticker.C:
			playing, err := client.PlayerCurrentlyPlaying(ctx)
			if err != nil {
				fmt.Fprintf(os.Stderr, "Error getting now playing info: %v", err)
				return
			}
			title := playingTitle(playing)
			if strings.HasPrefix(info, title) {
				continue
			}
			artists := playingArtists(playing)
			album := playingAlbum(playing)
			info = fmt.Sprintf("%v\n%v\n%v\n", title, artists, album)
			fmt.Fprintln(os.Stderr, "NEW TITLE")
			fmt.Fprintf(portWriter, "%v%v", info, delim)
		case <-req:
			fmt.Fprintln(os.Stderr, "READY")
			fmt.Fprintf(portWriter, "%v%v", info, delim)
		}
	}
}

func playingTitle(p *spotify.CurrentlyPlaying) string {
	if p == nil || p.Item == nil {
		return ""
	}
	return normalize(p.Item.Name)
}

func playingArtists(p *spotify.CurrentlyPlaying) string {
	if p == nil || p.Item == nil {
		return ""
	}
	var names []string
	for _, a := range p.Item.Artists {
		names = append(names, a.Name)
	}
	return normalize(strings.Join(names, ", "))
}

func playingAlbum(p *spotify.CurrentlyPlaying) string {
	if p == nil || p.Item == nil {
		return ""
	}
	return normalize(p.Item.Album.Name)
}

func normalize(str string) string {
	t := transform.Chain(norm.NFD, runes.Remove(runes.In(unicode.Mn)))
	ret, _, err := transform.String(t, str)
	if err != nil {
		return str
	}
	return ret
}

func wrtieClientTokenToFile(client *spotify.Client, path string) error {
	token, err := client.Token()
	if err != nil {
		return err
	}
	j, err := json.MarshalIndent(token, "", "  ")
	if err != nil {
		return err
	}
	return os.WriteFile(path, j, 0666)
}
